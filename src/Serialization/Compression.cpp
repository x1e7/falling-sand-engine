#include "Serialization/Compression.h"
#include <cstring>
#include <stdexcept>

static size_t fastlz_max_compressed_size(size_t size) {
    return size + (size / 255) + 16 + 2;
}

std::vector<uint8_t> Compression::compress(const uint8_t* data, size_t size) {
    if (size == 0) return {};

    size_t maxSize = fastlz_max_compressed_size(size);
    std::vector<uint8_t> output(maxSize + sizeof(uint32_t));

    uint32_t originalSize = static_cast<uint32_t>(size);
    std::memcpy(output.data(), &originalSize, sizeof(uint32_t));

    int compressedSize = fastlz_compress(
        data,
        static_cast<int>(size),
        output.data() + sizeof(uint32_t)
    );

    if (compressedSize <= 0) {
        throw std::runtime_error("FastLZ compression failed");
    }

    size_t totalSize = static_cast<size_t>(compressedSize) + sizeof(uint32_t);

    if (static_cast<float>(totalSize) / size > COMPRESSION_THRESHOLD) {
        std::vector<uint8_t> uncompressed(size + sizeof(uint32_t));
        uint32_t flag = 0xFFFFFFFF;
        std::memcpy(uncompressed.data(), &flag, sizeof(uint32_t));
        std::memcpy(uncompressed.data() + sizeof(uint32_t), data, size);
        return uncompressed;
    }

    output.resize(totalSize);
    return output;
}

std::vector<uint8_t> Compression::decompress(const uint8_t* data, size_t size) {
    if (size < sizeof(uint32_t)) {
        throw std::runtime_error("Invalid compressed data");
    }

    uint32_t originalSize;
    std::memcpy(&originalSize, data, sizeof(uint32_t));

    if (originalSize == 0xFFFFFFFF) {
        std::vector<uint8_t> result(size - sizeof(uint32_t));
        std::memcpy(result.data(), data + sizeof(uint32_t), size - sizeof(uint32_t));
        return result;
    }

    std::vector<uint8_t> output(originalSize);
    int result = fastlz_decompress(
        data + sizeof(uint32_t),
        static_cast<int>(size - sizeof(uint32_t)),
        output.data(),
        static_cast<int>(originalSize)
    );

    if (result <= 0) {
        throw std::runtime_error("FastLZ decompression failed");
    }

    output.resize(static_cast<size_t>(result));
    return output;
}
