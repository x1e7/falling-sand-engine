#include "Serialization/Compression.h"
#include "lz4.h"
#include <cstring>
#include <stdexcept>

std::vector<uint8_t> Compression::compress(const uint8_t* data, size_t size) {
    if (size == 0) return {};

    int maxCompressedSize = LZ4_compressBound(static_cast<int>(size));
    if (maxCompressedSize <= 0) {
        throw std::runtime_error("LZ4 compression bound calculation failed");
    }

    std::vector<uint8_t> output(sizeof(uint32_t) + maxCompressedSize);

    uint32_t originalSize = static_cast<uint32_t>(size);
    std::memcpy(output.data(), &originalSize, sizeof(uint32_t));

    int compressedSize = LZ4_compress_default(
        reinterpret_cast<const char*>(data),
        reinterpret_cast<char*>(output.data() + sizeof(uint32_t)),
        static_cast<int>(size),
        maxCompressedSize
    );

    if (compressedSize <= 0) {
        throw std::runtime_error("LZ4 compression failed");
    }

    size_t totalSize = sizeof(uint32_t) + static_cast<size_t>(compressedSize);

    if (static_cast<float>(totalSize) / size > COMPRESSION_THRESHOLD) {
        std::vector<uint8_t> uncompressed(sizeof(uint32_t) + size);
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

    int result = LZ4_decompress_safe(
        reinterpret_cast<const char*>(data + sizeof(uint32_t)),
        reinterpret_cast<char*>(output.data()),
        static_cast<int>(size - sizeof(uint32_t)),
        static_cast<int>(originalSize)
    );

    if (result < 0) {
        throw std::runtime_error("LZ4 decompression failed (malformed data)");
    }

    output.resize(static_cast<size_t>(result));
    return output;
}
