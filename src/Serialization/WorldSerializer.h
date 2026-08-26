#pragma once

#include "World/World.h"
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

/*struct WorldMetadata {
    TODO
};*/

class WorldSerializer {
public:
    static void saveWorld(const World& world, const std::string& fileName);

    static bool loadWorld(World& world, const std::string& fileName);
private:
    struct Header {
        uint32_t magic;
        uint32_t version;
        uint32_t width;
        uint32_t height;
        uint32_t dataSize;
        uint32_t compressedSize;
        uint32_t compressed;
        uint32_t reserved;
    };

    static std::vector<uint8_t> serializeParticles(const World& world);
    static bool deserializeParticles(World& world, const uint8_t* data, size_t size);
};
