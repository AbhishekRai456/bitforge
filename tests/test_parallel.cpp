#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>

#include "../src/buffered_io.hpp"
#include "../src/parallel_freq.hpp"

static std::string write_temp_file(const std::vector<uint8_t>& data,
                                   const std::string& name) {
    std::string path = "/tmp/" + name;
    FILE* fp = std::fopen(path.c_str(), "wb");
    assert(fp && "Failed to open temp file for writing");
    if (!data.empty()) {
        std::fwrite(data.data(), 1, data.size(), fp);
    }
    std::fclose(fp);
    return path;
}

static void test_buffered_read_roundtrip() {
    std::cout << "\n--- buffered read roundtrip ---\n";
    std::vector<uint8_t> original(1024);
    for (int i = 0; i < 1024; ++i) original[i] = static_cast<uint8_t>(i % 256);

    std::string path = write_temp_file(original, "test_buffered.bin");
    std::vector<uint8_t> result = read_file_buffered(path);

    assert(result.size() == original.size() && "Size mismatch after buffered read");
    assert(result == original && "Content mismatch after buffered read");

    std::cout << "  ok: test_buffered_read_roundtrip\n";
}

static void test_buffered_read_multi_chunk() {
    std::cout << "\n--- buffered read multi chunk ---\n";
    // Write exactly 3x the chunk size to force multiple fread calls.
    const std::size_t CHUNK = 256 * 1024;
    std::vector<uint8_t> original(3 * CHUNK);
    for (std::size_t i = 0; i < original.size(); ++i) {
        original[i] = static_cast<uint8_t>(i & 0xFF);
    }

    std::string path = write_temp_file(original, "test_multichunk.bin");
    std::vector<uint8_t> result = read_file_buffered(path, CHUNK);

    assert(result.size() == original.size() && "Multi-chunk size mismatch");
    assert(result == original && "Multi-chunk content mismatch");

    std::cout << "  ok: test_buffered_read_multi_chunk\n";
}

static void test_parallel_matches_single() {
    std::cout << "\n--- parallel matches single ---\n";
    std::vector<uint8_t> data;
    data.reserve(256 * 257 / 2);
    for (int i = 0; i < 256; ++i) {
        for (int j = 0; j <= i; ++j) {
            data.push_back(static_cast<uint8_t>(i));
        }
    }

    FreqTable single   = count_frequencies_single(data.data(), data.size());
    FreqTable parallel = count_frequencies_parallel(data.data(), data.size(), 4);

    for (int i = 0; i < 256; ++i) {
        assert(single[i] == parallel[i] && "Frequency mismatch between single and parallel");
    }

    uint64_t total = 0;
    for (int i = 0; i < 256; ++i) total += parallel[i];
    assert(total == data.size() && "Total frequency count != data size");

    std::cout << "  ok: test_parallel_matches_single\n";
}

static void test_parallel_single_thread_fallback() {
    std::cout << "\n--- parallel single thread fallback ---\n";
    std::vector<uint8_t> data = {'a', 'b', 'a', 'c', 'a', 'b'};

    FreqTable single   = count_frequencies_single(data.data(), data.size());
    FreqTable parallel = count_frequencies_parallel(data.data(), data.size(), 1);

    assert(single == parallel && "Single-thread fallback mismatch");
    std::cout << "  ok: test_parallel_single_thread_fallback\n";
}

static void test_parallel_more_threads_than_bytes() {
    std::cout << "\n--- parallel more threads than bytes ---\n";
    std::vector<uint8_t> data = {'x', 'y', 'z'};  // 3 bytes, 8 threads

    FreqTable single   = count_frequencies_single(data.data(), data.size());
    FreqTable parallel = count_frequencies_parallel(data.data(), data.size(), 8);

    assert(single == parallel && "More-threads-than-bytes edge case failed");
    std::cout << "  ok: test_parallel_more_threads_than_bytes\n";
}

static void test_parallel_empty_input() {
    std::cout << "\n--- parallel empty input ---\n";
    FreqTable result = count_frequencies_parallel(nullptr, 0, 4);
    for (int i = 0; i < 256; ++i) {
        assert(result[i] == 0 && "Empty input should give zero frequencies");
    }
    std::cout << "  ok: test_parallel_empty_input\n";
}

static void test_frequency_sum_equals_length() {
    std::cout << "\n--- frequency sum equals length ---\n";
    std::vector<uint8_t> data(100000);
    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<uint8_t>((i * 6364136223846793005ULL + 1) & 0xFF);
    }

    for (unsigned int threads : {1u, 2u, 4u, 8u}) {
        FreqTable ft = count_frequencies_parallel(data.data(), data.size(), threads);
        uint64_t total = 0;
        for (int i = 0; i < 256; ++i) total += ft[i];
        assert(total == data.size() && "Frequency sum != data length");
    }

    std::cout << "  ok: test_frequency_sum_equals_length\n";
}

#include "../src/encoder.hpp"

static void test_compress_decompress_endtoend() {
    std::cout << "\n--- compress decompress end-to-end ---\n";
    std::string input   = "/tmp/day3_input.txt";
    std::string compressed   = "/tmp/day3_compressed.huff";
    std::string decompressed = "/tmp/day3_decompressed.txt";

    const std::string content = "the quick brown fox jumps over the lazy dog "
                                "the quick brown fox jumps over the lazy dog";

    {
        FILE* fp = std::fopen(input.c_str(), "wb");
        assert(fp);
        std::fwrite(content.data(), 1, content.size(), fp);
        std::fclose(fp);
    }

    compress(input, compressed);
    decompress(compressed, decompressed);

    // Read back decompressed and compare byte-by-byte
    std::vector<uint8_t> result = read_file_buffered(decompressed);
    assert(result.size() == content.size() && "End-to-end size mismatch");
    for (std::size_t i = 0; i < content.size(); ++i) {
        assert(result[i] == static_cast<uint8_t>(content[i]) && "End-to-end byte mismatch");
    }

    std::cout << "  ok: test_compress_decompress_endtoend\n";
}

int main() {
    std::cout << "Running Day 3 buffered I/O & parallel tests...\n";

    test_buffered_read_roundtrip();
    test_buffered_read_multi_chunk();
    test_parallel_matches_single();
    test_parallel_single_thread_fallback();
    test_parallel_more_threads_than_bytes();
    test_parallel_empty_input();
    test_frequency_sum_equals_length();
    test_compress_decompress_endtoend();

    std::cout << "\nDone. 8/8 tests passed. Day 3 exit criteria met.\n";
    return 0;
}