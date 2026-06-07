#include "cli.hpp"

#include <cstring>
#include <stdexcept>
#include <string>
#include <cstdlib>
#include <iostream>

void print_usage(const char* program_name) {
    std::fprintf(stderr,
        "Usage: %s [OPTIONS]\n"
        "\n"
        "Modes (exactly one required):\n"
        "  --compress              Compress the input file\n"
        "  --decompress            Decompress the input file\n"
        "\n"
        "Options:\n"
        "  --input  <path>         Path to input file  (required)\n"
        "  --output <path>         Path to output file (required)\n"
        "  --threads <n>           Worker thread count (default: hardware auto-detect)\n"
        "  --no-stats              Suppress post-run statistics\n"
        "  --help                  Show this message and exit\n"
        "\n"
        "Examples:\n"
        "  %s --compress   --input data.txt  --output data.huff\n"
        "  %s --decompress --input data.huff --output data.txt\n"
        "  %s --compress   --input data.txt  --output data.huff --threads 4\n",
        program_name, program_name, program_name, program_name
    );
}

// Linear scan parser. Boolean flags set state, value flags consume the next argument.
// Throws on unknown flags or missing required fields.
CliArgs parse_args(int argc, char* argv[]) {
    CliArgs args;

    if (argc < 2) {
        throw std::runtime_error("No arguments provided.");
    }

    for (int i = 1; i < argc; ++i) {
        const char* token = argv[i];

        if (std::strcmp(token, "--help") == 0) {
            args.show_help = true;
            return args; 

        } else if (std::strcmp(token, "--compress") == 0) {
            if (args.mode != Mode::NONE) {
                throw std::runtime_error(
                    "Error: --compress and --decompress are mutually exclusive.");
            }
            args.mode = Mode::COMPRESS;

        } else if (std::strcmp(token, "--decompress") == 0) {
            if (args.mode != Mode::NONE) {
                throw std::runtime_error(
                    "Error: --compress and --decompress are mutually exclusive.");
            }
            args.mode = Mode::DECOMPRESS;

        } else if (std::strcmp(token, "--no-stats") == 0) {
            args.show_stats = false;

        } else if (std::strcmp(token, "--input") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error("Error: --input requires a file path argument.");
            }
            args.input_path = argv[++i];

        } else if (std::strcmp(token, "--output") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error("Error: --output requires a file path argument.");
            }
            args.output_path = argv[++i];

        } else if (std::strcmp(token, "--threads") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error("Error: --threads requires a numeric argument.");
            }
            const char* val = argv[++i];

            // Parse and validate that the entire string is a positive integer
            char* endptr = nullptr;
            unsigned long parsed = std::strtoul(val, &endptr, 10);

            if (endptr == val || *endptr != '\0' || parsed == 0) {
                throw std::runtime_error(
                    std::string("Error: --threads argument must be a positive integer, got: ")
                    + val);
            }

            args.thread_count = static_cast<unsigned int>(parsed);

        } else {
            throw std::runtime_error(
                std::string("Error: Unknown argument: ") + token +
                "\nRun with --help for usage.");
        }
    }

    if (args.show_help) return args; 

    if (args.mode == Mode::NONE) {
        throw std::runtime_error("Error: Must specify either --compress or --decompress.");
    }

    if (args.input_path.empty()) {
        throw std::runtime_error("Error: --input <path> is required.");
    }

    if (args.output_path.empty()) {
        throw std::runtime_error("Error: --output <path> is required.");
    }

    return args;
}