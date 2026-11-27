#pragma once

#include "World/World.h"

#include <string>
#include <vector>
#include <cstdint>

class WorldSerializer {
public:
    static bool saveWorld(const World& world, const std::string& fileName);
    static bool loadWorld(World& world, const std::string& fileName);

private:
    struct Header {
        uint32_t magic = 0;
        uint32_t version = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t dataSize = 0;
        uint32_t compressedSize = 0;
        uint32_t compressed = 0;
        uint32_t reserved = 0;
    };

    static std::vector<uint8_t> serializeParticles(const World& world);
};
