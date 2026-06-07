#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>

using FreqTable = std::array<uint64_t, 256>;

// Baseline sequential counter (used as fallback and test oracle)
FreqTable count_frequencies_single(const uint8_t* data, std::size_t length);

// Splits data into chunks for thread-local counting to avoid lock contention
FreqTable count_frequencies_parallel(const uint8_t* data,
                                      std::size_t length,
                                      unsigned int thread_count);