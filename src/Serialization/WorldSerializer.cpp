#include <Sand2D/Serialization/WorldSerializer.h>
#include <iostream>

namespace Sand2D {

static const uint32_t MAGIC = 0x53414E44; // "SAND"
static const uint32_t VERSION = 1;

void WorldSerializer::saveWorld(const World& world, const std::string& fileName)
{
    std::ofstream file(fileName, std::ios::binary);

    std::vector<uint8_t> rawData = serializeParticles(world);

    std::vector<uint8_t> compressedData = Compression::compress(
        rawData.data(),
        rawData.size()
    );

    Header header;
    header.magic = MAGIC;
    header.version = VERSION;
    header.width = world.getWidth();
    header.height = world.getHeight();
    header.dataSize = static_cast<uint32_t>(rawData.size());
    header.compressedSize = static_cast<uint32_t>(compressedData.size());
    header.compressed = 1;

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(compressedData.data()), compressedData.size());

    std::cout << "World saved to " << fileName << std::endl;
}

bool WorldSerializer::loadWorld(World& world, const std::string& fileName)
{
    std::ifstream file(fileName, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to read data from: " << fileName << std::endl;
        return false;
    }

    Header header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.magic != MAGIC) {
        std::cerr << "Invalid file format (magic mismatch): " << fileName << std::endl;
        return false;
    }

    if (header.version != VERSION) {
        std::cerr << "Unsupported version: " << header.version << std::endl;
        return false;
    }

    if (header.width != world.getWidth() || header.height != world.getHeight()) {
        std::cerr << "World size mismatch: expected "
                  << world.getWidth() << "x" << world.getHeight()
                  << ", got " << header.width << "x" << header.height << std::endl;
        return false;
    }

    std::vector<uint8_t> compressedData(header.compressedSize);
    file.read(reinterpret_cast<char*>(compressedData.data()), compressedData.size());

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

    if (!deserializeParticles(world, rawData.data(), rawData.size())) {
        std::cerr << "Deserialization failed" << std::endl;
        return false;
    }

    std::cout << "World loaded from " << fileName << std::endl;
    return true;
}

std::vector<uint8_t> WorldSerializer::serializeParticles(const World& world) {
    const int width = world.getWidth();
    const int height = world.getHeight();
    const size_t totalSize = width * height * 2; // id + temp

    std::vector<uint8_t> buffer(totalSize);
    uint8_t* ptr = buffer.data();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto& particle = world.getParticle(x, y);
            *ptr++ = particle.id;
            *ptr++ = particle.temp;
            //*ptr++ = particle.age;
        }
    }

    return buffer;
}

bool WorldSerializer::deserializeParticles(World& world, const uint8_t* data, size_t size) {
    const int width = world.getWidth();
    const int height = world.getHeight();
    const size_t expectedSize = width * height * 2;

    if (size != expectedSize) {
        std::cerr << "Particle data size mismatch";
        return false;
    }

    const uint8_t* ptr = data;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            ParticleId id = *ptr++;
            uint8_t temp = *ptr++;
            //uint8_t age = *ptr++;

            auto& p = world.getParticle(x, y);
            p.id = id;
            p.temp = temp;
            //p.age = age;
            world.markDirty(x, y);
        }
    }

    return true;
}

}
