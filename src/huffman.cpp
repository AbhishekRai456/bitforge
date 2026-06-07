#include "huffman.hpp"

#include <algorithm>
#include <iostream>
#include <queue>
#include <stdexcept>

std::unordered_map<uint8_t, uint64_t>
build_frequency_table(const uint8_t* data, size_t length) {
    std::unordered_map<uint8_t, uint64_t> table;
    table.reserve(256); 

    for (size_t i = 0; i < length; ++i) {
        ++table[data[i]];
    }
    return table;
}

HuffmanNode* build_tree(const std::unordered_map<uint8_t, uint64_t>& freq_table) {
    if (freq_table.empty()) {
        return nullptr;
    }

    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, NodeComparator> min_heap;

    for (const auto& [symbol, freq] : freq_table) {
        min_heap.push(new HuffmanNode(symbol, freq));
    }

    // return the lone leaf directly as the root
    if (min_heap.size() == 1) {
        return min_heap.top();
    }

    while (min_heap.size() > 1) {
        HuffmanNode* left  = min_heap.top(); min_heap.pop();
        HuffmanNode* right = min_heap.top(); min_heap.pop();

        // Create internal node with frequency equal to the sum of children.
        min_heap.push(new HuffmanNode(left->frequency + right->frequency, left, right));
    }

    return min_heap.top();
}

static void dfs(const HuffmanNode* node,
                const std::string& current_code,
                std::unordered_map<uint8_t, std::string>& code_map) {
    if (node == nullptr) return;

    if (node->is_leaf()) {
        // Handle single-symbol case by assigning a default bit.
        code_map[node->symbol] = current_code.empty() ? "0" : current_code;
        return;
    }

    dfs(node->left,  current_code + "0", code_map);
    dfs(node->right, current_code + "1", code_map);
}

void generate_codes(const HuffmanNode* root,
                    std::unordered_map<uint8_t, std::string>& code_map) {
    dfs(root, "", code_map);
}

// Post-order traversal ensures children are deleted before the parent.
void free_tree(HuffmanNode* node) {
    if (node == nullptr) return;
    free_tree(node->left);
    free_tree(node->right);
    delete node;
}

void print_codes(const std::unordered_map<uint8_t, std::string>& code_map) {
    std::vector<std::pair<uint8_t, std::string>> entries(code_map.begin(), code_map.end());
    std::sort(entries.begin(), entries.end(),
              [](const auto& a, const auto& b) { return a.second.size() < b.second.size(); });

    for (const auto& [sym, code] : entries) {
        if (sym >= 32 && sym < 127) {
            std::cout << "  '" << static_cast<char>(sym) << "'  (0x"
                      << std::hex << static_cast<int>(sym) << std::dec
                      << ")  freq-implied  code: " << code
                      << "  bits: " << code.size() << "\n";
        } else {
            std::cout << "  0x" << std::hex << static_cast<int>(sym) << std::dec
                      << "          code: " << code
                      << "  bits: " << code.size() << "\n";
        }
    }
}