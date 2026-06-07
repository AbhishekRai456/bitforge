#include <cassert>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../src/cli.hpp"

// Helper: build a fake argv array from a vector of string literals.
static std::vector<char*> make_argv(std::vector<std::string>& tokens) {
    std::vector<char*> argv;
    argv.reserve(tokens.size());
    for (auto& s : tokens) argv.push_back(s.data());
    return argv;
}

static void test_compress_basic() {
    std::vector<std::string> tokens = {
        "huffman", "--compress", "--input", "in.txt", "--output", "out.huff"
    };
    auto argv = make_argv(tokens);

    CliArgs args = parse_args(static_cast<int>(argv.size()), argv.data());

    assert(args.mode == Mode::COMPRESS   && "Mode should be COMPRESS");
    assert(args.input_path  == "in.txt"  && "Input path mismatch");
    assert(args.output_path == "out.huff" && "Output path mismatch");
    assert(args.thread_count == 0        && "Thread count should default to 0 (auto)");
    assert(args.show_stats == true       && "Stats should default to true");

    std::cout << "  ok: test_compress_basic\n";
}

static void test_decompress_basic() {
    std::vector<std::string> tokens = {
        "huffman", "--decompress", "--input", "data.huff", "--output", "data.txt"
    };
    auto argv = make_argv(tokens);

    CliArgs args = parse_args(static_cast<int>(argv.size()), argv.data());

    assert(args.mode == Mode::DECOMPRESS   && "Mode should be DECOMPRESS");
    assert(args.input_path  == "data.huff" && "Input path mismatch");
    assert(args.output_path == "data.txt"  && "Output path mismatch");

    std::cout << "  ok: test_decompress_basic\n";
}

static void test_threads_parsed() {
    std::vector<std::string> tokens = {
        "huffman", "--compress", "--input", "a.txt",
        "--output", "b.huff", "--threads", "4"
    };
    auto argv = make_argv(tokens);

    CliArgs args = parse_args(static_cast<int>(argv.size()), argv.data());

    assert(args.thread_count == 4 && "Thread count should be 4");
    std::cout << "  ok: test_threads_parsed\n";
}

static void test_no_stats_flag() {
    std::vector<std::string> tokens = {
        "huffman", "--compress", "--input", "a.txt",
        "--output", "b.huff", "--no-stats"
    };
    auto argv = make_argv(tokens);

    CliArgs args = parse_args(static_cast<int>(argv.size()), argv.data());

    assert(args.show_stats == false && "--no-stats should set show_stats to false");
    std::cout << "  ok: test_no_stats_flag\n";
}

static void test_help_flag() {
    std::vector<std::string> tokens = { "huffman", "--help" };
    auto argv = make_argv(tokens);

    CliArgs args = parse_args(static_cast<int>(argv.size()), argv.data());

    // --help should bypass validation and return early
    assert(args.show_help == true && "--help should set show_help");
    std::cout << "  ok: test_help_flag\n";
}

static void test_missing_input_throws() {
    std::vector<std::string> tokens = {
        "huffman", "--compress", "--output", "b.huff"
    };
    auto argv = make_argv(tokens);

    bool threw = false;
    try {
        parse_args(static_cast<int>(argv.size()), argv.data());
    } catch (const std::runtime_error& e) {
        threw = true;
        std::string msg(e.what());
        assert(msg.find("--input") != std::string::npos && "Error message should mention --input");
    }
    assert(threw && "Missing --input should throw");
    std::cout << "  ok: test_missing_input_throws\n";
}

static void test_missing_output_throws() {
    std::vector<std::string> tokens = {
        "huffman", "--compress", "--input", "a.txt"
    };
    auto argv = make_argv(tokens);

    bool threw = false;
    try {
        parse_args(static_cast<int>(argv.size()), argv.data());
    } catch (const std::runtime_error& e) {
        threw = true;
        std::string msg(e.what());
        assert(msg.find("--output") != std::string::npos && "Error message should mention --output");
    }
    assert(threw && "Missing --output should throw");
    std::cout << "  ok: test_missing_output_throws\n";
}

static void test_missing_mode_throws() {
    std::vector<std::string> tokens = {
        "huffman", "--input", "a.txt", "--output", "b.huff"
    };
    auto argv = make_argv(tokens);

    bool threw = false;
    try {
        parse_args(static_cast<int>(argv.size()), argv.data());
    } catch (const std::runtime_error& e) {
        threw = true;
    }
    assert(threw && "Missing mode should throw");
    std::cout << "  ok: test_missing_mode_throws\n";
}

static void test_mutually_exclusive_modes_throw() {
    std::vector<std::string> tokens = {
        "huffman", "--compress", "--decompress",
        "--input", "a.txt", "--output", "b.huff"
    };
    auto argv = make_argv(tokens);

    bool threw = false;
    try {
        parse_args(static_cast<int>(argv.size()), argv.data());
    } catch (const std::runtime_error& e) {
        threw = true;
    }
    assert(threw && "Mutually exclusive modes should throw");
    std::cout << "  ok: test_mutually_exclusive_modes_throw\n";
}

static void test_unknown_flag_throws() {
    std::vector<std::string> tokens = {
        "huffman", "--compress", "--input", "a.txt",
        "--output", "b.huff", "--frobnicate"
    };
    auto argv = make_argv(tokens);

    bool threw = false;
    try {
        parse_args(static_cast<int>(argv.size()), argv.data());
    } catch (const std::runtime_error& e) {
        threw = true;
        std::string msg(e.what());
        assert(msg.find("--frobnicate") != std::string::npos
               && "Error should mention the unknown flag");
    }
    assert(threw && "Unknown flag should throw");
    std::cout << "  ok: test_unknown_flag_throws\n";
}

static void test_threads_invalid_value_throws() {
    std::vector<std::string> tokens = {
        "huffman", "--compress", "--input", "a.txt",
        "--output", "b.huff", "--threads", "abc"
    };
    auto argv = make_argv(tokens);

    bool threw = false;
    try {
        parse_args(static_cast<int>(argv.size()), argv.data());
    } catch (const std::runtime_error& e) {
        threw = true;
    }
    assert(threw && "--threads abc should throw");
    std::cout << "  ok: test_threads_invalid_value_throws\n";
}

static void test_threads_zero_throws() {
    std::vector<std::string> tokens = {
        "huffman", "--compress", "--input", "a.txt",
        "--output", "b.huff", "--threads", "0"
    };
    auto argv = make_argv(tokens);

    bool threw = false;
    try {
        parse_args(static_cast<int>(argv.size()), argv.data());
    } catch (const std::runtime_error& e) {
        threw = true;
    }
    assert(threw && "--threads 0 should throw");
    std::cout << "  ok: test_threads_zero_throws\n";
}

static void test_input_missing_value_throws() {
    // --input is the last token, meaning no path is provided
    std::vector<std::string> tokens = {
        "huffman", "--compress", "--output", "b.huff", "--input"
    };
    auto argv = make_argv(tokens);

    bool threw = false;
    try {
        parse_args(static_cast<int>(argv.size()), argv.data());
    } catch (const std::runtime_error& e) {
        threw = true;
    }
    assert(threw && "--input with no value should throw");
    std::cout << "  ok: test_input_missing_value_throws\n";
}

int main() {
    std::cout << "Running Day 4 CLI tests...\n\n";

    test_compress_basic();
    test_decompress_basic();
    test_threads_parsed();
    test_no_stats_flag();
    test_help_flag();
    test_missing_input_throws();
    test_missing_output_throws();
    test_missing_mode_throws();
    test_mutually_exclusive_modes_throw();
    test_unknown_flag_throws();
    test_threads_invalid_value_throws();
    test_threads_zero_throws();
    test_input_missing_value_throws();

    std::cout << "\nDone. 13/13 passed.\n";
    return 0;
}