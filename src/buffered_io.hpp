#pragma once
#define _POSIX_C_SOURCE 200809L

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

// RAII wrapper to guarantee fclose runs if exceptions are thrown
struct FileHandle {
    FILE* fp = nullptr;

    explicit FileHandle(const std::string& path, const char* mode) {
        fp = std::fopen(path.c_str(), mode);
        if (!fp) {
            throw std::runtime_error("Cannot open file: " + path);
        }
    }

    ~FileHandle() {
        if (fp) std::fclose(fp);
    }

    FileHandle(const FileHandle&)            = delete;
    FileHandle& operator=(const FileHandle&) = delete;
    FileHandle(FileHandle&& other) noexcept : fp(other.fp) { other.fp = nullptr; }

    FILE* get() const { return fp; }
};

// 256KB chunk size balances syscall overhead and L2 cache limits
static constexpr std::size_t DEFAULT_CHUNK_SIZE = 256 * 1024;

std::vector<uint8_t> read_file_buffered(const std::string& path,
                                         std::size_t chunk_size = DEFAULT_CHUNK_SIZE);

int64_t get_file_size(const std::string& path);