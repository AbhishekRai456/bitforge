#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "../src/encoder.hpp"
#include "../src/buffered_io.hpp"

static void write_file(const std::string& path, const std::vector<uint8_t>& data) {
    FILE* fp = std::fopen(path.c_str(), "wb");
    assert(fp);
    if (!data.empty()) std::fwrite(data.data(), 1, data.size(), fp);
    std::fclose(fp);
}

static std::vector<uint8_t> read_file(const std::string& path) {
    return read_file_buffered(path);
}

static bool files_identical(const std::string& a, const std::string& b) {
    auto va = read_file(a);
    auto vb = read_file(b);
    return va == vb;
}

// Empty file compresses and decompresses without crashing.
static void test_empty_file() {
    const std::string in   = "/tmp/d5_empty_in.bin";
    const std::string comp = "/tmp/d5_empty.huff";
    const std::string out  = "/tmp/d5_empty_out.bin";

    write_file(in, {});

    compress(in, comp);
    decompress(comp, out);

    auto result = read_file(out);
    assert(result.empty() && "Empty file round-trip should produce empty output");

    std::cout << "  ok: test_empty_file\n";
}

// Single byte, single occurrence.
static void test_single_byte_single_occurrence() {
    const std::string in   = "/tmp/d5_single_in.bin";
    const std::string comp = "/tmp/d5_single.huff";
    const std::string out  = "/tmp/d5_single_out.bin";

    write_file(in, {0x42});

    compress(in, comp);
    decompress(comp, out);

    assert(files_identical(in, out) && "Single-byte file round-trip failed");
    std::cout << "  ok: test_single_byte_single_occurrence\n";
}

// Single unique symbol repeated many times (leaf-node edge case).
static void test_single_unique_symbol_repeated() {
    const std::string in   = "/tmp/d5_repeat_in.bin";
    const std::string comp = "/tmp/d5_repeat.huff";
    const std::string out  = "/tmp/d5_repeat_out.bin";

    std::vector<uint8_t> data(10000, 0xAB);
    write_file(in, data);

    compress(in, comp);
    decompress(comp, out);

    assert(files_identical(in, out) && "Single-symbol repeated round-trip failed");
    std::cout << "  ok: test_single_unique_symbol_repeated\n";
}

// All 256 byte values present exactly once.
static void test_all_256_byte_values() {
    const std::string in   = "/tmp/d5_all256_in.bin";
    const std::string comp = "/tmp/d5_all256.huff";
    const std::string out  = "/tmp/d5_all256_out.bin";

    std::vector<uint8_t> data(256);
    for (int i = 0; i < 256; ++i) data[i] = static_cast<uint8_t>(i);
    write_file(in, data);

    compress(in, comp);
    decompress(comp, out);

    assert(files_identical(in, out) && "All-256-values round-trip failed");
    std::cout << "  ok: test_all_256_byte_values\n";
}

// Binary file (not valid UTF-8) round-trips correctly.
static void test_binary_data_roundtrip() {
    const std::string in   = "/tmp/d5_binary_in.bin";
    const std::string comp = "/tmp/d5_binary.huff";
    const std::string out  = "/tmp/d5_binary_out.bin";

    std::vector<uint8_t> data(4096);
    for (int i = 0; i < 4096; ++i) {
        data[i] = static_cast<uint8_t>((i * 6364136223846793005ULL + 1442695040888963407ULL) & 0xFF);
    }
    write_file(in, data);

    compress(in, comp);
    decompress(comp, out);

    assert(files_identical(in, out) && "Binary data round-trip failed");
    std::cout << "  ok: test_binary_data_roundtrip\n";
}

// Corrupt magic number is rejected with an exception.
static void test_corrupt_magic_rejected() {
    const std::string bad  = "/tmp/d5_corrupt.huff";
    const std::string out  = "/tmp/d5_corrupt_out.bin";

    std::vector<uint8_t> fake = {'B','A','D','!', 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    write_file(bad, fake);

    bool threw = false;
    try {
        decompress(bad, out);
    } catch (const std::exception&) {
        threw = true;
    }

    assert(threw && "Corrupt magic should throw an exception");
    std::cout << "  ok: test_corrupt_magic_rejected\n";
}

// Compressed file is actually smaller than input for compressible data.
static void test_compression_ratio_on_skewed_input() {
    const std::string in   = "/tmp/d5_skewed_in.bin";
    const std::string comp = "/tmp/d5_skewed.huff";

    std::vector<uint8_t> data;
    data.reserve(10010);
    for (int i = 0; i < 10000; ++i) data.push_back('a');
    for (int i = 0; i < 10; ++i)    data.push_back('b');
    write_file(in, data);

    compress(in, comp);

    int64_t original_size   = static_cast<int64_t>(data.size());
    int64_t compressed_size = get_file_size(comp);

    assert(compressed_size < original_size &&
           "Skewed input should compress smaller than original");
    std::cout << "  ok: test_compression_ratio_on_skewed_input\n";
}

// Large file (1 MB) round-trips correctly.
static void test_large_file_roundtrip() {
    const std::string in   = "/tmp/d5_large_in.bin";
    const std::string comp = "/tmp/d5_large.huff";
    const std::string out  = "/tmp/d5_large_out.bin";

    std::vector<uint8_t> data(1024 * 1024);
    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<uint8_t>((i * 2654435761ULL) & 0xFF);
    }
    write_file(in, data);

    compress(in, comp);
    decompress(comp, out);

    assert(files_identical(in, out) && "Large file round-trip failed");
    std::cout << "  ok: test_large_file_roundtrip\n";
}

int main() {
    std::cout << "Running Day 5 edge case tests...\n\n";

    test_empty_file();
    test_single_byte_single_occurrence();
    test_single_unique_symbol_repeated();
    test_all_256_byte_values();
    test_binary_data_roundtrip();
    test_corrupt_magic_rejected();
    test_compression_ratio_on_skewed_input();
    test_large_file_roundtrip();

    std::cout << "\nDone. 8/8 tests passed.\n";
    return 0;
}