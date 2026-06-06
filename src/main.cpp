#include "huffman.hpp"

#include <iostream>
#include <string>

int main() {
    // Temporary test harness (logic will shift to a CLI parser in later iterations.)
    const std::string input = "hello huffman world";
    const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data());

    std::cout << "=== Huffman Engine — Day 1 Smoke Test ===\n";
    std::cout << "Input: \"" << input << "\"\n\n";

    auto freq_table = build_frequency_table(data, input.size());
    std::cout << "Frequency table (" << freq_table.size() << " unique symbols):\n";
    for (const auto& [sym, freq] : freq_table) {
        if (sym >= 32 && sym < 127)
            std::cout << "  '" << static_cast<char>(sym) << "' : " << freq << "\n";
        else
            std::cout << "  0x" << std::hex << static_cast<int>(sym)
                      << std::dec << " : " << freq << "\n";
    }

    HuffmanNode* root = build_tree(freq_table);
    std::cout << "\nTree root frequency: " << root->frequency << "\n";
    std::cout << "(Should equal input length: " << input.size() << ")\n\n";

    std::unordered_map<uint8_t, std::string> code_map;
    generate_codes(root, code_map);

    std::cout << "Generated codes (sorted by code length):\n";
    print_codes(code_map);

    bool all_covered = true;
    for (const auto& [sym, freq] : freq_table) {
        if (code_map.find(sym) == code_map.end()) {
            std::cerr << "ERROR: symbol 0x" << std::hex << static_cast<int>(sym)
                      << " has no code!\n";
            all_covered = false;
        }
    }
    if (all_covered) std::cout << "\n[PASS] All symbols have codes.\n";

    free_tree(root);
    std::cout << "[PASS] Tree memory released.\n";

    return 0;
}