#pragma once

#include <string>
#include <cstdint>
#include <functional>

// Called periodically with (processed, total) during heavy I/O operations
using ProgressCallback = std::function<void(uint64_t, uint64_t)>;

double compress(const std::string& input_path,
              const std::string& output_path,
              unsigned int thread_count = 0,
              ProgressCallback progress_cb = nullptr);

void decompress(const std::string& input_path,
                const std::string& output_path,
                ProgressCallback progress_cb = nullptr);