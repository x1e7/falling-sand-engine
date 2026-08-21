#pragma once

#include "Core/ParticleRegistry.h"
#include "Core/Math/Vector2.h"
#include "World/Chunk.h"
#include <vector>

class World {
public:
    World(int width, int height, ParticleRegistry& registry);
    ~World();

    void tick(float deltaTime);

    void setParticle(int x, int y, ParticleId id);

    bool isInside(int x, int y) const;

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    ParticleRegistry& getRegistry() { return m_registry; }
    const ParticleRegistry& getRegistry() const { return m_registry; }

    inline ParticleInstance* getParticlePtr(int x, int y) {
        int cx = x >> 4;
        int cy = y >> 4;
        int lx = x & 15;
        int ly = y & 15;
        return &m_chunks[cy * m_chunksX + cx].cells[ly * CHUNK_SIZE + lx];
    }

    inline const ParticleInstance* getParticlePtr(int x, int y) const {
        int cx = x >> 4;
        int cy = y >> 4;
        int lx = x & 15;
        int ly = y & 15;
        return &m_chunks[cy * m_chunksX + cx].cells[ly * CHUNK_SIZE + lx];
    }
private:
    int m_width;
    int m_height;

    Chunk* m_chunks;
    int m_chunksX, m_chunksY;

    bool* m_movedThisFrame;
    ParticleRegistry& m_registry;

    inline ParticleInstance& at(int x, int y) {
        int cx = x >> 4;
        int cy = y >> 4;
        int lx = x & 15;
        int ly = y & 15;
        return m_chunks[cy * m_chunksX + cx].cells[ly * CHUNK_SIZE + lx];
    }

    inline const ParticleInstance& at(int x, int y) const {
        int cx = x >> 4;
        int cy = y >> 4;
        int lx = x & 15;
        int ly = y & 15;
        return m_chunks[cy * m_chunksX + cx].cells[ly * CHUNK_SIZE + lx];
    }

    void wakeChunk(int x, int y);

    void updatePowder(const Vec2i& pos);
    void updateLiquid(const Vec2i& pos);
    void updateGas(const Vec2i& pos);
    void updateFire(const Vec2i& pos);

    bool canMove(const Vec2i& from, const Vec2i& to);
    void performSwap(const Vec2i& from, const Vec2i& to);

    float m_accumulator = 0.0f;
    static constexpr float FIXED_DT = 1.0f / 120.0f;
    static constexpr float VELOCITY_DAMPING = 0.98f;
    static constexpr float GRAVITY = 0.1f;
};
