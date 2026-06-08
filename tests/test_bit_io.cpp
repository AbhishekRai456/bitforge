#include "bit_io.hpp"
#include "encoder.hpp"
#include "huffman.hpp"

#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static int tests_run    = 0;
static int tests_passed = 0;

#define CHECK(condition, name)                                          \
    do {                                                                \
        ++tests_run;                                                    \
        if (condition) {                                                \
            std::cout << "  ok: " << (name) << "\n";                    \
            ++tests_passed;                                             \
        } else {                                                        \
            std::cerr << "  FAIL: " << (name) << "\n";                  \
        }                                                               \
    } while (0)

static void write_file(const std::string& path, const std::string& content) {
    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) throw std::runtime_error("Cannot create file: " + path);
    std::fwrite(content.data(), 1, content.size(), f);
    std::fclose(f);
}

static std::string read_file(const std::string& path) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return "";
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    std::rewind(f);
    if (sz <= 0) { std::fclose(f); return ""; }
    std::string result(static_cast<size_t>(sz), '\0');
    std::fread(result.data(), 1, static_cast<size_t>(sz), f);
    std::fclose(f);
    return result;
}

static bool files_identical(const std::string& a, const std::string& b) {
    return read_file(a) == read_file(b);
}

static void test_bit_io_isolation() {
    std::cout << "\n--- bit io isolation ---\n";
    const char* tmp_path = "/tmp/test_bit_io.bin";

    {
        std::FILE* f = std::fopen(tmp_path, "wb");
        BitWriter bw(f);
        bw.write_bit(1); bw.write_bit(0); bw.write_bit(1); bw.write_bit(1);
        bw.write_bit(0); bw.write_bit(0); bw.write_bit(1); bw.write_bit(0);
        bw.write_bit(1); bw.write_bit(1); bw.write_bit(1);
        uint8_t padding = bw.flush();
        CHECK(padding == 5, "flush padding is 5 bits");
        CHECK(bw.bits_written() == 11, "bits_written() == 11");
        std::fclose(f);
    }

    {
        std::FILE* f = std::fopen(tmp_path, "rb");
        BitReader br(f, 11);
        uint8_t expected[] = {1,0,1,1,0,0,1,0,1,1,1};
        bool all_correct = true;
        for (int i = 0; i < 11; ++i) {
            uint8_t bit = br.read_bit();
            // Read back the exact 11 bits we wrote.
            if (bit != expected[i]) {
                std::cerr << "  Bit " << i << ": expected " << (int)expected[i] << " got " << (int)bit << "\n";
                all_correct = false;
            }
        }
        CHECK(all_correct, "11 bits read back correctly");
        CHECK(br.exhausted(), "reader is exhausted after 11 bits");
        std::fclose(f);
    }
}

static void test_byte_roundtrip() {
    std::cout << "\n--- byte roundtrip ---\n";
    const char* tmp_path = "/tmp/test_byte_rt.bin";
    const uint8_t test_bytes[] = {0x00, 0xFF, 0xA5, 0x5A, 0x01, 0x80, 0x7F};
    const int n = sizeof(test_bytes);

    {
        std::FILE* f = std::fopen(tmp_path, "wb");
        BitWriter bw(f);
        for (int i = 0; i < n; ++i) bw.write_byte(test_bytes[i]);
        bw.flush();
        std::fclose(f);
    }

    {
        std::FILE* f = std::fopen(tmp_path, "rb");
        BitReader br(f, static_cast<uint64_t>(n) * 8);
        bool all_correct = true;
        for (int i = 0; i < n; ++i) {
            uint8_t got = br.read_byte();
            if (got != test_bytes[i]) {
                std::cerr << "  Byte " << i << ": expected 0x" << std::hex << (int)test_bytes[i] << " got 0x" << (int)got << std::dec << "\n";
                all_correct = false;
            }
        }
        CHECK(all_correct, "all 7 test bytes read back correctly");
        std::fclose(f);
    }
}

static void test_roundtrip_text() {
    std::cout << "\n--- text roundtrip ---\n";
    const std::string original = "the quick brown fox jumps over the lazy dog";
    write_file("/tmp/rt_original.txt", original);
    compress("/tmp/rt_original.txt", "/tmp/rt_compressed.huff");
    decompress("/tmp/rt_compressed.huff", "/tmp/rt_decompressed.txt");
    CHECK(files_identical("/tmp/rt_original.txt", "/tmp/rt_decompressed.txt"), "text roundtrip identical");
}

static void test_roundtrip_repetitive() {
    std::cout << "\n--- repetitive input ---\n";
    std::string original(10000, 'a');
    for (size_t i = 0; i < original.size(); i += 7) original[i] = 'b';
    write_file("/tmp/rt_rep.txt", original);
    compress("/tmp/rt_rep.txt", "/tmp/rt_rep.huff");
    decompress("/tmp/rt_rep.huff", "/tmp/rt_rep_out.txt");
    CHECK(files_identical("/tmp/rt_rep.txt", "/tmp/rt_rep_out.txt"), "repetitive input roundtrip identical");

    std::FILE* compressed = std::fopen("/tmp/rt_rep.huff", "rb");
    std::fseek(compressed, 0, SEEK_END);
    long compressed_size = std::ftell(compressed);
    std::fclose(compressed);
    // Compression should help on highly repetitive data.
    CHECK(compressed_size < (long)original.size(), "repetitive input smaller than original");
}

static void test_roundtrip_binary() {
    std::cout << "\n--- binary data ---\n";
    std::vector<uint8_t> original;
    for (int rep = 0; rep < 4; ++rep) {
        for (int i = 0; i < 256; ++i) original.push_back(static_cast<uint8_t>(i));
    }

    {
        std::FILE* f = std::fopen("/tmp/rt_binary.bin", "wb");
        std::fwrite(original.data(), 1, original.size(), f);
        std::fclose(f);
    }

    compress("/tmp/rt_binary.bin", "/tmp/rt_binary.huff");
    decompress("/tmp/rt_binary.huff", "/tmp/rt_binary_out.bin");
    CHECK(files_identical("/tmp/rt_binary.bin", "/tmp/rt_binary_out.bin"), "binary data roundtrip identical");
}

static void test_roundtrip_single_symbol() {
    std::cout << "\n--- single-symbol input ---\n";
    write_file("/tmp/rt_single.txt", std::string(500, 'z'));
    compress("/tmp/rt_single.txt", "/tmp/rt_single.huff");
    decompress("/tmp/rt_single.huff", "/tmp/rt_single_out.txt");
    CHECK(files_identical("/tmp/rt_single.txt", "/tmp/rt_single_out.txt"), "single symbol roundtrip identical");
}

static void test_roundtrip_empty() {
    std::cout << "\n--- empty file ---\n";
    write_file("/tmp/rt_empty.txt", "");
    compress("/tmp/rt_empty.txt", "/tmp/rt_empty.huff");
    decompress("/tmp/rt_empty.huff", "/tmp/rt_empty_out.txt");
    CHECK(files_identical("/tmp/rt_empty.txt", "/tmp/rt_empty_out.txt"), "empty file roundtrip identical");
}

static void test_corrupt_magic_rejected() {
    std::cout << "\n--- corrupt file handling ---\n";
    write_file("/tmp/rt_corrupt.huff", "BADMAGIC\x00\x00\x00\x00\x00\x00\x00\x00");
    bool threw = false;
    try {
        decompress("/tmp/rt_corrupt.huff", "/tmp/rt_corrupt_out.txt");
    } catch (const std::exception&) {
        threw = true;
    }
    CHECK(threw, "corrupt file throws exception");
}

int main() {
    std::cout << "Running Huffman compression tests...\n";
    test_bit_io_isolation();
    test_byte_roundtrip();
    test_roundtrip_text();
    test_roundtrip_repetitive();
    test_roundtrip_binary();
    test_roundtrip_single_symbol();
    test_roundtrip_empty();
    test_corrupt_magic_rejected();
    std::cout << "\nDone. " << tests_passed << "/" << tests_run << " passed.\n";
    return (tests_passed == tests_run) ? 0 : 1;
}