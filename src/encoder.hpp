#pragma once

#include <cstdint>
#include <string>

// Returns compression ratio (compressed / original)
double compress(const std::string& src_path, const std::string& dst_path);

void decompress(const std::string& src_path, const std::string& dst_path);