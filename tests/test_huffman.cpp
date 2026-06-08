#include "huffman.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

static int tests_run    = 0;
static int tests_passed = 0;

#define CHECK(condition, name)                                          \
    do {                                                                \
        ++tests_run;                                                    \
        if (condition) {                                                \
            std::cout << "  ok: " << (name) << "\n";                    \
            ++tests_passed;                                             \
        } else {                                                        \
            std::cerr << "  FAIL: " << (name) << "\n";                  \
        }                                                               \
    } while (0)

static void test_frequency_table() {
    std::cout << "\n--- build_frequency_table ---\n";

    const std::string s = "aabbbcccc";
    auto freq = build_frequency_table(
        reinterpret_cast<const uint8_t*>(s.data()), s.size());

    CHECK(freq.size() == 3,     "exactly 3 unique symbols");
    CHECK(freq[uint8_t('a')] == 2, "'a' frequency == 2");
    CHECK(freq[uint8_t('b')] == 3, "'b' frequency == 3");
    CHECK(freq[uint8_t('c')] == 4, "'c' frequency == 4");
}

static void test_tree_root_frequency() {
    std::cout << "\n--- build_tree root frequency ---\n";

    const std::string s = "hello huffman";
    auto freq = build_frequency_table(
        reinterpret_cast<const uint8_t*>(s.data()), s.size());

    HuffmanNode* root = build_tree(freq);
    CHECK(root != nullptr, "tree root is not null");
    CHECK(root->frequency == s.size(), "root->frequency == input length");

    free_tree(root);
}

static void test_code_coverage() {
    std::cout << "\n--- generate_codes coverage ---\n";

    const std::string s = "abracadabra";
    auto freq = build_frequency_table(
        reinterpret_cast<const uint8_t*>(s.data()), s.size());

    HuffmanNode* root = build_tree(freq);
    std::unordered_map<uint8_t, std::string> codes;
    generate_codes(root, codes);

    bool all_covered = true;
    for (const auto& [sym, f] : freq) {
        if (codes.find(sym) == codes.end()) {
            all_covered = false;
            std::cerr << "  Missing code for symbol: " << static_cast<char>(sym) << "\n";
        }
    }
    CHECK(all_covered, "every input symbol has a code");
    CHECK(codes.size() == freq.size(), "code map size == unique symbol count");

    free_tree(root);
}

static void test_prefix_free() {
    std::cout << "\n--- prefix-free property ---\n";

    const std::string s = "the quick brown fox jumps over the lazy dog";
    auto freq = build_frequency_table(
        reinterpret_cast<const uint8_t*>(s.data()), s.size());

    HuffmanNode* root = build_tree(freq);
    std::unordered_map<uint8_t, std::string> codes;
    generate_codes(root, codes);

    bool prefix_free = true;
    std::vector<std::string> all_codes;
    all_codes.reserve(codes.size());
    for (const auto& [sym, code] : codes) all_codes.push_back(code);

    // Verify the prefix-free property: no code can be a prefix of another.
    for (size_t i = 0; i < all_codes.size() && prefix_free; ++i) {
        for (size_t j = 0; j < all_codes.size() && prefix_free; ++j) {
            if (i == j) continue;
            const std::string& shorter = (all_codes[i].size() < all_codes[j].size())
                                       ? all_codes[i] : all_codes[j];
            const std::string& longer  = (all_codes[i].size() < all_codes[j].size())
                                       ? all_codes[j] : all_codes[i];
            if (longer.substr(0, shorter.size()) == shorter) {
                std::cerr << "  Prefix violation: \"" << shorter
                          << "\" is prefix of \"" << longer << "\"\n";
                prefix_free = false;
            }
        }
    }
    CHECK(prefix_free, "no code is a prefix of another");

    free_tree(root);
}

static void test_single_symbol() {
    std::cout << "\n--- single-symbol input ---\n";

    const std::string s = "aaaaaaa";
    auto freq = build_frequency_table(
        reinterpret_cast<const uint8_t*>(s.data()), s.size());

    HuffmanNode* root = build_tree(freq);
    std::unordered_map<uint8_t, std::string> codes;
    generate_codes(root, codes);

    CHECK(codes.size() == 1, "single symbol: exactly one code entry");
    CHECK(!codes[uint8_t('a')].empty(), "single symbol: code is non-empty");

    free_tree(root);
}

static void test_frequency_ordering() {
    std::cout << "\n--- frequency length ordering ---\n";

    // Frequent symbols should have shorter bit representations.
    const std::string s = "aaaaaaaaaab"; 
    auto freq = build_frequency_table(
        reinterpret_cast<const uint8_t*>(s.data()), s.size());

    HuffmanNode* root = build_tree(freq);
    std::unordered_map<uint8_t, std::string> codes;
    generate_codes(root, codes);

    size_t len_a = codes[uint8_t('a')].size();
    size_t len_b = codes[uint8_t('b')].size();

    CHECK(len_a <= len_b, "more frequent 'a' has code length <= less frequent 'b'");

    free_tree(root);
}

int main() {
    std::cout << "Running Huffman tests...\n";

    test_frequency_table();
    test_tree_root_frequency();
    test_code_coverage();
    test_prefix_free();
    test_single_symbol();
    test_frequency_ordering();

    std::cout << "\nDone. " << tests_passed << "/" << tests_run << " passed.\n";

    return (tests_passed == tests_run) ? 0 : 1;
}