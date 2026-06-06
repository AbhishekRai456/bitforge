#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct HuffmanNode {
    uint8_t symbol;
    uint64_t frequency;
    HuffmanNode* left;
    HuffmanNode* right;

    // Leaf node representing a byte value.
    HuffmanNode(uint8_t sym, uint64_t freq)
        : symbol(sym), frequency(freq), left(nullptr), right(nullptr) {}

    // Internal node created by merging two children.
    HuffmanNode(uint64_t freq, HuffmanNode* l, HuffmanNode* r)
        : symbol(0), frequency(freq), left(l), right(r) {}

    bool is_leaf() const { return left == nullptr && right == nullptr; }
};

// std::priority_queue is a max-heap by default;
// reverse comparison to obtain min-frequency extraction.
struct NodeComparator {
    bool operator()(const HuffmanNode* a, const HuffmanNode* b) const {
        return a->frequency > b->frequency;
    }
};

std::unordered_map<uint8_t, uint64_t> build_frequency_table(const uint8_t* data, size_t length);
HuffmanNode* build_tree(const std::unordered_map<uint8_t, uint64_t>& freq_table);
void generate_codes(const HuffmanNode* root, std::unordered_map<uint8_t, std::string>& code_map);
void free_tree(HuffmanNode* node);
void print_codes(const std::unordered_map<uint8_t, std::string>& code_map);