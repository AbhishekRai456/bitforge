#include "encoder.hpp"
#include "bit_io.hpp"
#include "huffman.hpp"
#include "buffered_io.hpp"
#include "parallel_freq.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <thread>

// .huff file signature
static constexpr uint8_t MAGIC[8] = {'H','U','F','F', 0x00, 0x01, 0x00, 0x00};

// Serialize tree structure via DFS (internal=0, leaf=1 + symbol)
static void serialize_tree(const HuffmanNode* node, BitWriter& bw) {
    if (node == nullptr) return;

    if (node->is_leaf()) {
        bw.write_bit(1);
        bw.write_byte(node->symbol);
    } else {
        bw.write_bit(0);
        serialize_tree(node->left,  bw);
        serialize_tree(node->right, bw);
    }
}

// Reconstruct tree from bitstream
static HuffmanNode* deserialize_tree(BitReader& br) {
    if (br.exhausted()) {
        throw std::runtime_error("deserialize_tree: bitstream exhausted unexpectedly");
    }

    uint8_t bit = br.read_bit();

    if (bit == 1) {
        uint8_t symbol = br.read_byte();
        return new HuffmanNode(symbol, 0); 
    } else {
        HuffmanNode* left  = deserialize_tree(br);
        HuffmanNode* right = deserialize_tree(br);
        return new HuffmanNode(0, left, right);
    }
}

// Little-endian uint64_t I/O
static void write_u64_le(std::FILE* f, uint64_t value) {
    uint8_t buf[8];
    for (int i = 0; i < 8; ++i) {
        buf[i] = static_cast<uint8_t>(value & 0xFFu);
        value >>= 8;
    }
    if (std::fwrite(buf, 1, 8, f) != 8) {
        throw std::runtime_error("write_u64_le: fwrite failed");
    }
}

static uint64_t read_u64_le(std::FILE* f) {
    uint8_t buf[8];
    if (std::fread(buf, 1, 8, f) != 8) {
        throw std::runtime_error("read_u64_le: fread failed");
    }
    uint64_t value = 0;
    for (int i = 7; i >= 0; --i) {
        value = (value << 8) | buf[i];
    }
    return value;
}

double compress(const std::string& src_path, const std::string& dst_path, unsigned int thread_count, ProgressCallback progress_cb) {
    std::vector<uint8_t> buffer = read_file_buffered(src_path);
    size_t file_size = buffer.size();

    if (file_size == 0) {
        FileHandle dst(dst_path, "wb");
        std::fwrite(MAGIC, 1, 8, dst.fp);
        write_u64_le(dst.fp, 0); 
        return 0.0;
    }

    if (thread_count == 0) {
        thread_count = std::thread::hardware_concurrency();
        if (thread_count == 0) thread_count = 1;
    }

    FreqTable freq_array = count_frequencies_parallel(buffer.data(), buffer.size(), thread_count);

    // Convert dense array to sparse map for the tree builder
    std::unordered_map<uint8_t, uint64_t> freq_table;
    for (int i = 0; i < 256; ++i) {
        if (freq_array[i] > 0) {
            freq_table[static_cast<uint8_t>(i)] = freq_array[i];
        }
    }
    HuffmanNode* root = build_tree(freq_table);

    std::unordered_map<uint8_t, std::string> code_map;
    generate_codes(root, code_map);

    // Write compressed file
    FileHandle dst(dst_path, "wb");

    if (std::fwrite(MAGIC, 1, 8, dst.fp) != 8) {
        throw std::runtime_error("compress: failed to write magic");
    }

    // Reserve space for payload bit count to be written later
    long bit_count_offset = std::ftell(dst.fp);
    write_u64_le(dst.fp, 0ULL); 

    // Serialize tree
    {
        BitWriter bw(dst.fp);
        serialize_tree(root, bw);
        bw.flush(); 
    }

    // Write compressed payload
    uint64_t payload_bit_count = 0;
    {
        for (const auto& byte_val : buffer) {
            payload_bit_count += code_map.at(byte_val).size();
        }

        BitWriter bw(dst.fp);
        static constexpr uint64_t PROGRESS_INTERVAL = 4096;
        uint64_t bytes_encoded = 0;

        for (const auto& byte_val : buffer) {
            const std::string& code = code_map.at(byte_val);
            for (char c : code) {
                bw.write_bit(static_cast<uint8_t>(c - '0'));
            }
            
            ++bytes_encoded;
            if (progress_cb && (bytes_encoded % PROGRESS_INTERVAL == 0)) {
                progress_cb(bytes_encoded, buffer.size());
            }
        }
        bw.flush();
    }

    if (progress_cb) {
        progress_cb(buffer.size(), buffer.size());
    }

    // Overwrite payload bit count
    if (std::fseek(dst.fp, bit_count_offset, SEEK_SET) != 0) {
        throw std::runtime_error("compress: fseek to bit_count_offset failed");
    }
    write_u64_le(dst.fp, payload_bit_count);

    free_tree(root);

    if (std::fseek(dst.fp, 0, SEEK_END) != 0) {
        throw std::runtime_error("compress: final fseek failed");
    }
    long compressed_size = std::ftell(dst.fp);

    return (compressed_size > 0 && file_size > 0)
        ? static_cast<double>(compressed_size) / static_cast<double>(file_size)
        : 1.0;
}

void decompress(const std::string& src_path, const std::string& dst_path, ProgressCallback progress_cb) {
    FileHandle src(src_path, "rb");

    // Validate magic
    uint8_t magic_buf[8];
    if (std::fread(magic_buf, 1, 8, src.fp) != 8) {
        throw std::runtime_error("decompress: file too short to contain magic");
    }
    if (std::memcmp(magic_buf, MAGIC, 8) != 0) {
        throw std::runtime_error("decompress: invalid magic number — not a .huff file");
    }

    uint64_t payload_bit_count = read_u64_le(src.fp);

    if (payload_bit_count == 0) {
        FileHandle dst(dst_path, "wb");
        return;
    }

    // Read the rest of the file into memory to avoid tricky mixed bit/byte I/O
    long header_end = std::ftell(src.fp); 
    if (std::fseek(src.fp, 0, SEEK_END) != 0) {
        throw std::runtime_error("decompress: fseek failed");
    }
    long total_size = std::ftell(src.fp);
    if (std::fseek(src.fp, header_end, SEEK_SET) != 0) {
        throw std::runtime_error("decompress: fseek to body failed");
    }

    size_t body_size = static_cast<size_t>(total_size - header_end);
    std::vector<uint8_t> body(body_size);
    if (std::fread(body.data(), 1, body_size, src.fp) != body_size) {
        throw std::runtime_error("decompress: failed to read body");
    }

    std::FILE* mem_file = ::fmemopen(body.data(), body_size, "rb");
    if (!mem_file) {
        throw std::runtime_error("decompress: fmemopen failed");
    }

    // Tree is self-delimiting, give it a large bit budget
    BitReader tree_reader(mem_file, body_size * 8ULL);
    HuffmanNode* root = deserialize_tree(tree_reader);

    // serialize_tree flushes to a byte boundary, so ftell gives the start of payload data
    long payload_byte_offset = std::ftell(mem_file);
    std::fclose(mem_file);

    if (payload_byte_offset < 0 || static_cast<size_t>(payload_byte_offset) > body_size) {
        free_tree(root);
        throw std::runtime_error("decompress: invalid payload offset");
    }

    uint8_t* payload_start = body.data() + payload_byte_offset;
    size_t   payload_size  = body_size - static_cast<size_t>(payload_byte_offset);

    std::FILE* payload_file = ::fmemopen(payload_start, payload_size, "rb");
    if (!payload_file) {
        free_tree(root);
        throw std::runtime_error("decompress: fmemopen for payload failed");
    }

    // Decode payload using the tree
    BitReader payload_reader(payload_file, payload_bit_count);
    std::vector<uint8_t> output;
    output.reserve(payload_bit_count / 4); // rough pre-allocation

    // Edge case: root is a leaf (single unique symbol in input)
    if (root->is_leaf()) {
        while (!payload_reader.exhausted()) {
            payload_reader.read_bit(); // consume the bit
            output.push_back(root->symbol);
        }
    } else {
        // Normal case: walk the tree bit-by-bit
        const HuffmanNode* current = root;
        while (!payload_reader.exhausted()) {
            uint8_t bit = payload_reader.read_bit();
            if (bit == 0) {
                current = current->left;
            } else {
                current = current->right;
            }

            if (current == nullptr) {
                std::fclose(payload_file);
                free_tree(root);
                throw std::runtime_error("decompress: traversal reached null node — corrupt file");
            }

            if (current->is_leaf()) {
                output.push_back(current->symbol);
                current = root; // Reset to root for the next symbol
            }
        }
    }

    std::fclose(payload_file);
    free_tree(root);

    // Write decompressed output
    FileHandle dst(dst_path, "wb");
    if (!output.empty()) {
        if (std::fwrite(output.data(), 1, output.size(), dst.fp) != output.size()) {
            throw std::runtime_error("decompress: fwrite failed");
        }
    }

    if (progress_cb) {
        progress_cb(1, 1);
    }
}