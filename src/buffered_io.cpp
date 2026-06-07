#define _POSIX_C_SOURCE 200809L

#include "buffered_io.hpp"
#include <stdexcept>
#include <cstring>

int64_t get_file_size(const std::string& path) {
    FileHandle fh(path, "rb");
    if (std::fseek(fh.get(), 0, SEEK_END) != 0) return -1;
    long size = std::ftell(fh.get());
    return static_cast<int64_t>(size);
}

std::vector<uint8_t> read_file_buffered(const std::string& path,
                                         std::size_t chunk_size) {
    FileHandle fh(path, "rb");
    FILE* fp = fh.get();

    std::vector<uint8_t> buffer;
    int64_t file_size = -1;

    // Try to pre-allocate exact capacity to avoid reallocations.
    // fseek will fail on pipes/stdin, so we just fall back to vector growth if it does.
    if (std::fseek(fp, 0, SEEK_END) == 0) {
        long sz = std::ftell(fp);
        if (sz >= 0) {
            file_size = static_cast<int64_t>(sz);
            buffer.reserve(static_cast<std::size_t>(file_size));
        }
        std::rewind(fp); 
    }

    std::vector<uint8_t> chunk(chunk_size);
    std::size_t bytes_read = 0;

    // Block read to minimize syscall overhead
    while ((bytes_read = std::fread(chunk.data(), 1, chunk_size, fp)) > 0) {
        buffer.insert(buffer.end(), chunk.data(), chunk.data() + bytes_read);
    }

    if (std::ferror(fp)) {
        throw std::runtime_error("fread error while reading: " + path);
    }

    return buffer;
}