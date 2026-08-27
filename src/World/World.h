#pragma once

#include "Core/ParticleRegistry.h"
#include "Core/Math/Vector2.h"
#include "World/Chunk.h"
#include <memory>

#include <random>

class World {
public:
    World(int width, int height, ParticleRegistry& registry);
    ~World() = default;

    void tick(float deltaTime);
    void setParticle(int x, int y, ParticleId id, uint8_t age = 0);
    bool isInside(int x, int y) const;

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    ParticleRegistry& getRegistry() { return m_registry; }
    const ParticleRegistry& getRegistry() const { return m_registry; }

    ParticleInstance* getParticlePtr(int x, int y) {
        int cx = x >> 4, cy = y >> 4;
        int lx = x & 15, ly = y & 15;
        return &m_chunks[cy * m_chunksX + cx].cells[ly * CHUNK_SIZE + lx];
    }

    const ParticleInstance* getParticlePtr(int x, int y) const {
        int cx = x >> 4, cy = y >> 4;
        int lx = x & 15, ly = y & 15;
        return &m_chunks[cy * m_chunksX + cx].cells[ly * CHUNK_SIZE + lx];
    }

private:
    // ====== CORE DATA ======
    int m_width, m_height;
    int m_chunksX, m_chunksY;
    std::unique_ptr<Chunk[]> m_chunks;
    std::unique_ptr<bool[]> m_movedThisFrame;
    ParticleRegistry& m_registry;

    // RANDOM
    std::uniform_int_distribution<int> m_distDir{-1, 1};
    std::uniform_int_distribution<int> m_distChance{0, 99};

    // ====== CACHED PARTICLE IDs ======
    ParticleId m_smokeId;
    ParticleId m_fireId;
    const ParticleDefinition* m_defCache[256];

    // ====== HELPERS ======
    ParticleInstance& at(int x, int y) {
        int cx = x >> 4, cy = y >> 4;
        int lx = x & 15, ly = y & 15;
        return m_chunks[cy * m_chunksX + cx].cells[ly * CHUNK_SIZE + lx];
    }

    const ParticleInstance& at(int x, int y) const {
        int cx = x >> 4, cy = y >> 4;
        int lx = x & 15, ly = y & 15;
        return m_chunks[cy * m_chunksX + cx].cells[ly * CHUNK_SIZE + lx];
    }

    void wakeChunk(int x, int y);

    inline bool canMove(const Vec2i& from, const Vec2i& to, const ParticleDefinition& fromDef);
    inline void performSwap(const Vec2i& from, const Vec2i& to);

    // ====== TIMING ======
    float m_accumulator = 0.0f;
    static constexpr float FIXED_DT = 1.0f / 120.0f;
};
