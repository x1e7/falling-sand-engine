#pragma once

#include <vector>
#include <cstdint>

#include "fastlz.h"

class Compression
{
public:
    static std::vector<uint8_t> compress(const uint8_t* data, size_t size);
    static std::vector<uint8_t> decompress(const uint8_t* data, size_t size);
private:
    static constexpr float COMPRESSION_THRESHOLD = 0.9f;
};
