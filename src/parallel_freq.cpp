#include "parallel_freq.hpp"

#include <thread>
#include <vector>
#include <algorithm>
#include <stdexcept>

FreqTable count_frequencies_single(const uint8_t* data, std::size_t length) {
    FreqTable freq{};
    freq.fill(0);
    for (std::size_t i = 0; i < length; ++i) {
        ++freq[data[i]];
    }
    return freq;
}

// Thread-local counting avoids mutexes and false sharing.
static void count_chunk(const uint8_t* begin,
                         const uint8_t* end,
                         FreqTable& out) {
    FreqTable local{};
    local.fill(0);

    for (const uint8_t* p = begin; p != end; ++p) {
        ++local[*p];
    }

    out = local;
}

FreqTable count_frequencies_parallel(const uint8_t* data,
                                      std::size_t length,
                                      unsigned int thread_count) {
    if (length == 0) {
        FreqTable empty{};
        empty.fill(0);
        return empty;
    }
    if (thread_count <= 1) {
        return count_frequencies_single(data, length);
    }

    if (thread_count == 0) thread_count = 1;
    if (static_cast<std::size_t>(thread_count) > length) {
        thread_count = static_cast<unsigned int>(length);
    }

    std::size_t chunk_size = length / thread_count;

    std::vector<FreqTable> results(thread_count);
    for (auto& ft : results) ft.fill(0);

    std::vector<std::thread> threads;
    threads.reserve(thread_count);

    for (unsigned int t = 0; t < thread_count; ++t) {
        const uint8_t* begin = data + t * chunk_size;
        // Last thread absorbs any remainder from integer division
        const uint8_t* end   = (t == thread_count - 1)
                                    ? data + length        
                                    : begin + chunk_size;

        threads.emplace_back(count_chunk, begin, end, std::ref(results[t]));
    }

    for (auto& th : threads) {
        th.join();
    }

    // Reduce thread-local tables into the final table
    FreqTable final_freq{};
    final_freq.fill(0);

    for (const auto& local : results) {
        for (int i = 0; i < 256; ++i) {
            final_freq[i] += local[i];
        }
    }

    return final_freq;
}