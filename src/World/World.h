#pragma once

#include "Core/Particle.h"
#include "Core/Math.h"
#include "World/Chunk.h"

#include <cstdint>
#include <memory>

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

    ParticleInstance* getParticlePtr(int x, int y) { return &at(x, y); }
    const ParticleInstance* getParticlePtr(int x, int y) const { return &at(x, y); }

    void loadParticles(const uint8_t* data, size_t size);

    int getTicksLastFrame() const { return m_ticksLastFrame; }
    int getActiveChunks() const { return m_activeChunks; }
    int getTotalChunks() const { return m_chunksX * m_chunksY; }

private:
    int m_width, m_height;
    int m_chunksX, m_chunksY;
    std::unique_ptr<Chunk[]> m_chunks;
    std::unique_ptr<uint32_t[]> m_movedThisFrame;
    int m_movedWords;
    ParticleRegistry& m_registry;

    uint32_t m_rngState = 0x9E3779B9u;
    ParticleId m_smokeId = ParticleRegistry::Empty;
    ParticleId m_fireId  = ParticleRegistry::Empty;

    int m_frameCounter = 0;
    int m_ticksLastFrame = 0;
    int m_activeChunks = 0;
    float m_accumulator = 0.0f;
    static constexpr float FIXED_DT = 1.0f / 120.0f;
    static constexpr int IDLE_THRESHOLD = 300;

    uint32_t nextRand() {
        uint32_t x = m_rngState;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        m_rngState = x;
        return x;
    }
    int randDir() { return static_cast<int>(nextRand() % 3u) - 1; }
    int randChance() { return static_cast<int>((nextRand() >> 8) % 100u); }
    uint8_t randBrightness() { return static_cast<uint8_t>(nextRand() >> 24); }

    bool isMoved(int index) const {
        return (m_movedThisFrame[index >> 5] & (1u << (index & 31))) != 0;
    }
    void setMoved(int index) {
        m_movedThisFrame[index >> 5] |= (1u << (index & 31));
    }

    ParticleInstance& at(int x, int y) {
        return m_chunks[(y >> CHUNK_SHIFT) * m_chunksX + (x >> CHUNK_SHIFT)]
            .cells[(y & CHUNK_MASK) * CHUNK_SIZE + (x & CHUNK_MASK)];
    }
    const ParticleInstance& at(int x, int y) const {
        return m_chunks[(y >> CHUNK_SHIFT) * m_chunksX + (x >> CHUNK_SHIFT)]
            .cells[(y & CHUNK_MASK) * CHUNK_SIZE + (x & CHUNK_MASK)];
    }

    static Vec2i velocityToDirection(int8_t vx, int8_t vy) {
        return {
            (vx > 0) ? 1 : (vx < 0) ? -1 : 0,
            (vy > 0) ? 1 : (vy < 0) ? -1 : 0
        };
    }

    void wakeChunk(int x, int y);
    bool canMove(const Vec2i& to, ParticleId fromId);
    void performSwap(const Vec2i& from, const Vec2i& to);
    void updateVelocity(int x, int y, const Vec2i* dirs, int count, ParticleInstance& p);
    bool tryMoveInDirection(int x, int y, Vec2i dir, ParticleId fromId);

    void updatePowder(int x, int y, ParticleInstance& p);
    void updateLiquid(int x, int y, ParticleInstance& p, const ParticlePhysics& ph);
    void updateGas   (int x, int y, ParticleInstance& p);
    void updateFire  (int x, int y, ParticleInstance& p);

    bool tryMeltSelf    (int x, int y, ParticleInstance& p, const ParticlePhysics& ph);
    bool tryMeltNeighbor(int x, int y);
    bool tryCorrode     (int x, int y, ParticleInstance& p);
    void tryIgnite      (int x, int y);

    void updateCell(int x, int y);
};
