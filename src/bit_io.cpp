#include "bit_io.hpp"

#include <cstring>
#include <stdexcept>

BitWriter::BitWriter(std::FILE* file)
    : file_(file), current_byte_(0), bit_count_(0), bits_written_(0) {}

void BitWriter::write_bit(uint8_t bit) {
    // Pack bits MSB-first
    current_byte_ = static_cast<uint8_t>((current_byte_ << 1) | (bit & 1u));
    ++bit_count_;
    ++bits_written_;

    if (bit_count_ == 8) {
        if (std::fputc(current_byte_, file_) == EOF) {
            throw std::runtime_error("BitWriter: fputc failed");
        }
        current_byte_ = 0;
        bit_count_    = 0;
    }
}

void BitWriter::write_byte(uint8_t byte) {
    for (int i = 7; i >= 0; --i) {
        write_bit((byte >> i) & 1u);
    }
}

uint8_t BitWriter::flush() {
    if (bit_count_ == 0) {
        return 0; 
    }

    // Left-shift to pad zeroes onto the LSB side
    uint8_t padding = static_cast<uint8_t>(8 - bit_count_);
    current_byte_ = static_cast<uint8_t>(current_byte_ << padding);

    if (std::fputc(current_byte_, file_) == EOF) {
        throw std::runtime_error("BitWriter: fputc failed during flush");
    }

    current_byte_ = 0;
    bit_count_    = 0;

    return padding;
}

BitReader::BitReader(std::FILE* file, uint64_t total_bits)
    : file_(file), current_byte_(0), bits_in_buffer_(0), bits_remaining_(total_bits) {}

uint8_t BitReader::read_bit() {
    if (bits_remaining_ == 0) {
        return 0; 
    }

    if (bits_in_buffer_ == 0) {
        int c = std::fgetc(file_);
        if (c == EOF) {
            throw std::runtime_error("BitReader: unexpected EOF before bit count exhausted");
        }
        current_byte_   = static_cast<uint8_t>(c);
        bits_in_buffer_ = 8;
    }

    // Extract MSB, then shift left to queue the next bit
    uint8_t bit = (current_byte_ >> 7) & 1u;
    current_byte_ = static_cast<uint8_t>(current_byte_ << 1);
    --bits_in_buffer_;
    --bits_remaining_;

    return bit;
}

uint8_t BitReader::read_byte() {
    uint8_t result = 0;
    for (int i = 7; i >= 0; --i) {
        result |= static_cast<uint8_t>(read_bit() << i);
    }
    return result;
}