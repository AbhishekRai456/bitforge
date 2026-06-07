#include "encoder.hpp"
#include "huffman.hpp"

#include <cstdio>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // Minimal CLI harness for testing core compression logic.
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <compress|decompress> <input> <output>\n";
        return 1;
    }

    const std::string mode   = argv[1];
    const std::string input  = argv[2];
    const std::string output = argv[3];

    try {
        if (mode == "compress") {
            double ratio = compress(input, output);
            std::cout << "Compressed: " << input << " -> " << output << "\n";
            std::printf("Ratio: %.4f (%.1f%% of original)\n", ratio, ratio * 100.0);
        } else if (mode == "decompress") {
            decompress(input, output);
            std::cout << "Decompressed: " << input << " -> " << output << "\n";
        } else {
            std::cerr << "Unknown mode: " << mode << "\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}