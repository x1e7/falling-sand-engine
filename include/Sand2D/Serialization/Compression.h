#pragma once

#include <vector>
#include <cstdint>

extern "C" {
#include "Sand2D/third_party/fastlz/fastlz.h"
}

namespace Sand2D {

class Compression
{
public:
    static std::vector<uint8_t> compress(const uint8_t* data, size_t size);
    static std::vector<uint8_t> decompress(const uint8_t* data, size_t size);
private:
    static constexpr float COMPRESSION_THRESHOLD = 0.9f;
};

}
