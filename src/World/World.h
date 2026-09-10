#pragma once

#include "Core/Particle.h"
#include "Core/Math.h"
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

    void loadParticles(const uint8_t* data, size_t size);

private:
    int m_width, m_height;
    int m_chunksX, m_chunksY;
    std::unique_ptr<Chunk[]> m_chunks;
    std::unique_ptr<uint32_t[]> m_movedThisFrame;
    int m_movedWords;
    ParticleRegistry& m_registry;

    std::mt19937 m_rng{std::random_device{}()};
    std::uniform_int_distribution<int> m_distDir{-1, 1};
    std::uniform_int_distribution<int> m_distChance{0, 99};

    ParticleId m_smokeId;
    ParticleId m_fireId;

    bool isMoved(int index) const {
        return (m_movedThisFrame[index >> 5] & (1u << (index & 31))) != 0;
    }
    void setMoved(int index) {
        m_movedThisFrame[index >> 5] |= (1u << (index & 31));
    }

    ParticleInstance& at(int x, int y) {
        return m_chunks[(y >> 4) * m_chunksX + (x >> 4)].cells[(y & 15) * CHUNK_SIZE + (x & 15)];
    }
    const ParticleInstance& at(int x, int y) const {
        return m_chunks[(y >> 4) * m_chunksX + (x >> 4)].cells[(y & 15) * CHUNK_SIZE + (x & 15)];
    }

    void wakeChunk(int x, int y);

    bool canMove(const Vec2i& from, const Vec2i& to, const ParticleDefinition& fromDef);
    void performSwap(const Vec2i& from, const Vec2i& to);

    bool tryMove(int x, int y, const Vec2i* dirs, int count, const ParticleDefinition& def);

    void updatePowder(int x, int y, ParticleInstance& p, const ParticleDefinition& def);
    void updateLiquid(int x, int y, ParticleInstance& p, const ParticleDefinition& def);
    void updateGas   (int x, int y, ParticleInstance& p, const ParticleDefinition& def);
    void updateFire  (int x, int y, ParticleInstance& p, const ParticleDefinition& def);

    void tryMeltSelf    (int x, int y, ParticleInstance& p, const ParticleDefinition& def);
    void tryMeltNeighbor(int x, int y, ParticleInstance& p);
    void tryCorrode     (int x, int y, ParticleInstance& p);
    void tryIgnite      (int x, int y);

    void updateCell(int x, int y);

    float m_accumulator = 0.0f;
    static constexpr float FIXED_DT = 1.0f / 120.0f;
};
