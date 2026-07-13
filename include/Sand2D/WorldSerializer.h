#pragma once

#include <Sand2D/World.h>
#include <fstream>
#include <string>
#include <iostream>

namespace Sand2D {

/*struct WorldMetadata {
    TODO
};*/

const uint32_t defaultMagic = 0x53414E44; // "SAND"

class WorldSerializer {
public:
    static void saveWorld(const World& world, const std::string& fileName)
    {
        std::ofstream file(fileName, std::ios::binary);

        uint32_t magic = defaultMagic; // "SAND"
        uint32_t version = 1;
        uint32_t width = world.getWidth();
        uint32_t height = world.getHeight();

        file.write(reinterpret_cast<const char*>(&magic), 4);
        file.write(reinterpret_cast<const char*>(&version), 4);
        file.write(reinterpret_cast<const char*>(&width), 4);
        file.write(reinterpret_cast<const char*>(&height), 4);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const auto& particle = world.getParticle(x, y);
                ParticleId id = particle.id;
                float temp = particle.temperature;
                file.write(reinterpret_cast<const char*>(&id), sizeof(ParticleId));
                file.write(reinterpret_cast<const char*>(&temp), sizeof(float));
            }
        }

        std::cout << "World saved to " << fileName << std::endl;
    }

    static bool loadWorld(World& world, const std::string& fileName)
    {
        std::ifstream file(fileName, std::ios::binary);
        if (!file) return false;

        uint32_t magic, version, width, height;
        file.read(reinterpret_cast<char*>(&magic), 4);
        if (magic != defaultMagic) return false;

        file.read(reinterpret_cast<char*>(&version), 4);
        file.read(reinterpret_cast<char*>(&width), 4);
        file.read(reinterpret_cast<char*>(&height), 4);

        if (width != world.getWidth() || height != world.getHeight()) {
            return false;
        }

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                ParticleId id;
                float temp;
                file.read(reinterpret_cast<char*>(&id), sizeof(ParticleId));
                file.read(reinterpret_cast<char*>(&temp), sizeof(float));
                world.setParticle(x, y, id, temp);
            }
        }

        std::cout << "World loaded from " << fileName << std::endl;

        return true;
    }
};

}
