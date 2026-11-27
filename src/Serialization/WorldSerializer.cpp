#include "Serialization/WorldSerializer.h"
#include "Serialization/Compression.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <system_error>

static constexpr uint32_t MAGIC = 0x53414E44;   // "SAND"
static constexpr uint32_t VERSION = 1;

bool WorldSerializer::saveWorld(const World& world, const std::string& fileName) {
    const std::string tmpName = fileName + ".tmp";

    std::vector<uint8_t> rawData = serializeParticles(world);
    std::vector<uint8_t> compressedData;
    try {
        compressedData = Compression::compress(rawData.data(), rawData.size());
    } catch (const std::exception& e) {
        std::cerr << "Compression failed: " << e.what() << std::endl;
        return false;
    }

    Header header;
    header.magic = MAGIC;
    header.version = VERSION;
    header.width = static_cast<uint32_t>(world.getWidth());
    header.height = static_cast<uint32_t>(world.getHeight());
    header.dataSize = static_cast<uint32_t>(rawData.size());
    header.compressedSize = static_cast<uint32_t>(compressedData.size());
    header.compressed = 1;
    header.reserved = 0;

    {
        std::ofstream file(tmpName, std::ios::binary | std::ios::trunc);
        if (!file) {
            std::cerr << "Failed to open for writing: " << tmpName << std::endl;
            return false;
        }
        file.write(reinterpret_cast<const char*>(&header), sizeof(header));
        file.write(reinterpret_cast<const char*>(compressedData.data()),
                   static_cast<std::streamsize>(compressedData.size()));
        file.flush();
        if (!file) {
            std::cerr << "Failed to write: " << tmpName << std::endl;
            return false;
        }
    }

    std::error_code ec;
    std::filesystem::rename(tmpName, fileName, ec);
    if (ec) {
        std::filesystem::remove(fileName, ec);
        ec.clear();
        std::filesystem::rename(tmpName, fileName, ec);
        if (ec) {
            std::cerr << "Failed to replace " << fileName << ": " << ec.message() << std::endl;
            std::filesystem::remove(tmpName, ec);
            return false;
        }
    }

    std::cout << "World saved to " << fileName << std::endl;
    return true;
}

bool WorldSerializer::loadWorld(World& world, const std::string& fileName) {
    std::error_code existsEc;
    if (!std::filesystem::exists(fileName, existsEc)) {
        std::cout << "No save found, starting with an empty world" << std::endl;
        return false;
    }

    std::ifstream file(fileName, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to read data from: " << fileName << std::endl;
        return false;
    }

    Header header;
    if (!file.read(reinterpret_cast<char*>(&header), sizeof(header))) {
        std::cerr << "Truncated header: " << fileName << std::endl;
        return false;
    }

    if (header.magic != MAGIC) {
        std::cerr << "Invalid file format (magic mismatch): " << fileName << std::endl;
        return false;
    }
    if (header.version != VERSION) {
        std::cerr << "Unsupported version: " << header.version << std::endl;
        return false;
    }
    if (header.width != static_cast<uint32_t>(world.getWidth()) ||
        header.height != static_cast<uint32_t>(world.getHeight())) {
        std::cerr << "World size mismatch: expected "
                  << world.getWidth() << "x" << world.getHeight()
                  << ", got " << header.width << "x" << header.height << std::endl;
        return false;
    }

    const size_t expectedRaw = static_cast<size_t>(world.getWidth()) * world.getHeight() * 2;
    if (header.dataSize != expectedRaw || header.compressedSize == 0) {
        std::cerr << "Corrupted save (size fields mismatch): " << fileName << std::endl;
        return false;
    }

    std::vector<uint8_t> compressedData(header.compressedSize);
    if (!file.read(reinterpret_cast<char*>(compressedData.data()),
                   static_cast<std::streamsize>(compressedData.size()))) {
        std::cerr << "Truncated payload: " << fileName << std::endl;
        return false;
    }

    std::vector<uint8_t> rawData;
    if (header.compressed) {
        try {
            rawData = Compression::decompress(compressedData.data(), compressedData.size());
        } catch (const std::exception& e) {
            std::cerr << "Decompression failed: " << e.what() << std::endl;
            return false;
        }
    } else {
        rawData = std::move(compressedData);
    }

    if (rawData.size() != expectedRaw) {
        std::cerr << "Corrupted save (payload size mismatch): " << fileName << std::endl;
        return false;
    }

    world.loadParticles(rawData.data(), rawData.size());
    std::cout << "World loaded from " << fileName << std::endl;
    return true;
}

std::vector<uint8_t> WorldSerializer::serializeParticles(const World& world) {
    const int width = world.getWidth();
    const int height = world.getHeight();
    const size_t totalSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 2;

    std::vector<uint8_t> buffer(totalSize);
    uint8_t* ptr = buffer.data();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const ParticleInstance* p = world.getParticlePtr(x, y);
            *ptr++ = p->id;
            *ptr++ = p->age;
        }
    }
    return buffer;
}
