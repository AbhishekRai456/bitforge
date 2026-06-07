#define _POSIX_C_SOURCE 200809L

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include "cli.hpp"
#include "encoder.hpp"
#include "progress.hpp"
#include "buffered_io.hpp"

// Print stats to stderr to avoid corrupting piped stdout.
static void print_stats(const CliArgs& args,
                         double elapsed_ms,
                         unsigned int thread_count) {
    std::fprintf(stderr, "\n--- Huffman Engine Statistics ---\n");
    std::fprintf(stderr, "  Mode        : %s\n",
        args.mode == Mode::COMPRESS ? "Compress" : "Decompress");
    std::fprintf(stderr, "  Input       : %s\n", args.input_path.c_str());
    std::fprintf(stderr, "  Output      : %s\n", args.output_path.c_str());
    std::fprintf(stderr, "  Threads     : %u\n", thread_count);
    std::fprintf(stderr, "  Elapsed     : %.2f ms\n", elapsed_ms);

    if (args.mode == Mode::COMPRESS) {
        int64_t input_size  = get_file_size(args.input_path);
        int64_t output_size = get_file_size(args.output_path);

        if (input_size > 0 && output_size > 0) {
            double ratio = static_cast<double>(output_size) /
                           static_cast<double>(input_size) * 100.0;
            double space_saving = 100.0 - ratio;

            std::fprintf(stderr, "  Input size  : %lld bytes\n",
                static_cast<long long>(input_size));
            std::fprintf(stderr, "  Output size : %lld bytes\n",
                static_cast<long long>(output_size));
            std::fprintf(stderr, "  Ratio       : %.1f%% of original (%.1f%% space saving)\n",
                ratio, space_saving);
        }
    }

    std::fprintf(stderr, "---------------------------------\n");
}

int main(int argc, char* argv[]) {
    CliArgs args;
    try {
        args = parse_args(argc, argv);
    } catch (const std::runtime_error& e) {
        std::fprintf(stderr, "%s\n\n", e.what());
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (args.show_help) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    // Auto-detect hardware threads if not specified
    unsigned int thread_count = args.thread_count;
    if (thread_count == 0) {
        thread_count = std::thread::hardware_concurrency();
        if (thread_count == 0) thread_count = 1;
    }

    auto t_start = std::chrono::steady_clock::now();

    try {
        if (args.mode == Mode::COMPRESS) {
            int64_t input_size = get_file_size(args.input_path);
            uint64_t total = (input_size > 0) ? static_cast<uint64_t>(input_size) : 0;

            ProgressBar bar("Compressing", total);

            // Pass progress callback to update the bar during encoding
            ProgressCallback cb = [&bar](uint64_t done, uint64_t total_bytes) {
                bar.update(done);
                (void)total_bytes;
            };

            // Explicitly cast to void to silence unused-return warnings 
            // while preserving the 'double' return type in the backend API.
            (void)compress(args.input_path, args.output_path, thread_count, cb);
            bar.finish();

        } else {
            // Show a simple working message for decompression since byte-level progress isn't hooked up yet
            std::fprintf(stderr, "Decompressing: %s ...\r", args.input_path.c_str());
            std::fflush(stderr);

            decompress(args.input_path, args.output_path, nullptr);

            // Clear the working line
            std::fprintf(stderr, "\r%80s\r", "");
            std::fflush(stderr);
        }

    } catch (const std::exception& e) {
        std::fprintf(stderr, "\nError: %s\n", e.what());
        return EXIT_FAILURE;
    }

    auto t_end = std::chrono::steady_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

    if (args.show_stats) {
        print_stats(args, elapsed_ms, thread_count);
    }

    return EXIT_SUCCESS;
}