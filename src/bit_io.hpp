#pragma once

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <vector>

// Accumulates bits MSB-first into a buffer.
class BitWriter {
public:
    explicit BitWriter(std::FILE* file);

    void write_bit(uint8_t bit);

    // Writes 8 bits, MSB first.
    void write_byte(uint8_t byte);

    // Flushes partial byte (zero-padded on the right). 
    // Returns padding bits added (0-7). Must be called at end of stream.
    uint8_t flush();

    // Total payload bits (excluding padding).
    uint64_t bits_written() const { return bits_written_; }

private:
    std::FILE* file_;
    uint8_t    current_byte_;
    int        bit_count_;
    uint64_t   bits_written_;
};

// Reads bits one at a time, stopping exactly at the total_payload_bits
// limit so padding is never consumed.
class BitReader {
public:
    // total_bits restricts reading to prevent consuming flush padding.
    explicit BitReader(std::FILE* file, uint64_t total_bits);

    // Returns 0 if exhausted; check exhausted() first.
    uint8_t read_bit();

    // Reads 8 bits, MSB first.
    uint8_t read_byte();

    bool exhausted() const { return bits_remaining_ == 0; }

private:
    std::FILE* file_;
    uint8_t    current_byte_;
    int        bits_in_buffer_;
    uint64_t   bits_remaining_;
};