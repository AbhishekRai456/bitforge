#pragma once

#include <string>
#include <cstdint>

enum class Mode {
    COMPRESS,
    DECOMPRESS,
    NONE  // Used to detect if the user forgot to specify a mode flag
};

struct CliArgs {
    Mode         mode         = Mode::NONE;
    std::string  input_path;
    std::string  output_path;
    unsigned int thread_count = 0;  // 0 = auto (hardware_concurrency)
    bool         show_stats   = true;
    bool         show_help    = false;
};

// Throws std::runtime_error on malformed input or missing required flags.
// If --help is passed, it suppresses errors and sets show_help = true.
CliArgs parse_args(int argc, char* argv[]);

void print_usage(const char* program_name);