#include "World/World.h"
#include <algorithm>
#include <cstring>

World::World(int width, int height, ParticleRegistry& registry)
    : m_width(width), m_height(height), m_registry(registry) {
    m_chunksX = (width + CHUNK_SIZE - 1) >> CHUNK_SHIFT;
    m_chunksY = (height + CHUNK_SIZE - 1) >> CHUNK_SHIFT;
    m_chunks = std::make_unique<Chunk[]>(static_cast<size_t>(m_chunksX) * m_chunksY);

    const size_t totalCells = static_cast<size_t>(width) * height;
    m_movedWords = (totalCells + 31) / 32;
    m_movedThisFrame = std::make_unique<uint32_t[]>(m_movedWords);

    m_smokeId = registry.findId("Smoke");
    m_fireId = registry.findId("Fire");
}

void World::loadParticles(const uint8_t* data, size_t size) {
    if (size != static_cast<size_t>(m_width) * m_height * 2) return;

    const uint8_t* ptr = data;
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            ParticleInstance& p = at(x, y);
            p.id = *ptr++;
            p.age = *ptr++;
            p.brightness = (p.id != ParticleRegistry::Empty) ? randBrightness() : 0;
        }
    }
}

void World::tick(float deltaTime) {
    m_accumulator = std::min(m_accumulator + deltaTime, 0.1f);
    m_ticksLastFrame = 0;
    m_activeChunks = 0;

    while (m_accumulator >= FIXED_DT) {
        std::memset(m_movedThisFrame.get(), 0, m_movedWords * sizeof(uint32_t));
        m_activeList.clear();

        // Phase 1: Collect fresh activity sources and increment idle counters
        for (int cy = 0; cy < m_chunksY; ++cy) {
            for (int cx = 0; cx < m_chunksX; ++cx) {
                Chunk& chunk = m_chunks[cy * m_chunksX + cx];
                if (chunk.idleFrames <= IDLE_THRESHOLD) {
                    // idleFrames==0 means real movement/brush happened this or last tick
                    if (chunk.idleFrames == 0) {
                        m_activeList.push_back({cx, cy});
                    }
                    chunk.idleFrames++;
                    m_activeChunks++;
                }
            }
        }

        // Phase 2: Wake direct neighbors of sources ONLY (no chain reaction)
        for (const auto& pos : m_activeList) {
            wakeNeighbors(pos.x, pos.y);
        }

        // Phase 3: Simulate all active chunks
        for (int cy = 0; cy < m_chunksY; ++cy) {
            for (int cx = 0; cx < m_chunksX; ++cx) {
                Chunk& chunk = m_chunks[cy * m_chunksX + cx];
                if (chunk.idleFrames > IDLE_THRESHOLD) continue;

                const int baseX = cx << CHUNK_SHIFT;
                const int baseY = cy << CHUNK_SHIFT;
                const int maxX = std::min(baseX + CHUNK_SIZE, m_width);
                const int maxY = std::min(baseY + CHUNK_SIZE, m_height);

                // Always bottom-to-top; horizontal randomness via randDir()
                for (int y = maxY - 1; y >= baseY; --y) {
                    for (int x = baseX; x < maxX; ++x) {
                        updateCell(x, y);
                    }
                }
            }
        }

        m_accumulator -= FIXED_DT;
        m_ticksLastFrame++;
    }
}

void World::updateCell(int x, int y) {
    const int idx = y * m_width + x;
    if (isMoved(idx)) return;

    ParticleInstance& p = at(x, y);
    if (p.id == ParticleRegistry::Empty) {
        p.age = 0;
        return;
    }

    if (p.age < 255) ++p.age;

    const ParticlePhysics& ph = m_registry.physics(p.id);
    if ((ph.flags & ParticleFlags::CanMelt) && tryMeltSelf(x, y, p, ph)) return;

    switch (ph.state) {
        case PhysicalState::Powder: updatePowder(x, y, p); break;
        case PhysicalState::Liquid: updateLiquid(x, y, p, ph); break;
        case PhysicalState::Gas:    updateGas(x, y, p); break;
        case PhysicalState::Fire:   updateFire(x, y, p); break;
        default: break;
    }
}

void World::updatePowder(int x, int y, ParticleInstance& p) {
    const int dx = randDir();
    const Vec2i dirs[3] = {{0, 1}, {dx, 1}, {-dx, 1}};
    tryMove(x, y, p, dirs, 3);
}

void World::updateLiquid(int x, int y, ParticleInstance& p, const ParticlePhysics& ph) {
    if ((ph.flags & ParticleFlags::IsCorrosive) && tryCorrode(x, y, p)) return;
    if ((ph.flags & ParticleFlags::IsHot) && tryMeltNeighbor(x, y)) return;

    const int dx = randDir();
    const Vec2i dirs[5] = {{0, 1}, {dx, 1}, {-dx, 1}, {dx, 0}, {-dx, 0}};
    tryMove(x, y, p, dirs, 5);
}

void World::updateGas(int x, int y, ParticleInstance& p) {
    if (randChance() < 5) {
        p.id = ParticleRegistry::Empty;
        p.age = 0;
        wakeChunk(x, y);
        return;
    }
    const int dx = randDir();
    const Vec2i dirs[5] = {{0, -1}, {dx, -1}, {-dx, -1}, {dx, 0}, {-dx, 0}};
    tryMove(x, y, p, dirs, 5);
}

void World::updateFire(int x, int y, ParticleInstance& p) {
    if (p.age > 20 && randChance() < (65 + p.age)) {
        p.id = (m_smokeId != ParticleRegistry::Empty) ? m_smokeId : ParticleRegistry::Empty;
        p.age = 0;
        p.brightness = randBrightness();
        wakeChunk(x, y);
        return;
    }

    const int dx = randDir();
    const Vec2i dirs[3] = {{0, -1}, {dx, -1}, {-dx, -1}};
    tryMove(x, y, p, dirs, 3);

    if (randChance() < 15) tryIgnite(x, y);
}

bool World::tryMove(int x, int y, ParticleInstance& p, const Vec2i* dirs, int count) {
    for (int i = 0; i < count; ++i) {
        const int nx = x + dirs[i].x;
        const int ny = y + dirs[i].y;
        if (!isInside(nx, ny)) continue;

        const int nIdx = ny * m_width + nx;
        if (isMoved(nIdx)) continue;

        if (m_registry.canDisplace(p.id, at(nx, ny).id)) {
            performSwap({x, y}, {nx, ny});
            return true;
        }
    }
    return false;
}

bool World::tryMeltSelf(int x, int y, ParticleInstance& p, const ParticlePhysics& ph) {
    if (ph.meltInto == ParticleRegistry::Empty || randChance() >= 10) return false;

    for (int dy = -1; dy <= 0; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (!dx && !dy) continue;
            const int nx = x + dx, ny = y + dy;
            if (!isInside(nx, ny)) continue;
            if (m_registry.physics(at(nx, ny).id).flags & ParticleFlags::IsHot) {
                p.id = ph.meltInto;
                p.age = 0;
                p.brightness = randBrightness();
                wakeChunk(x, y);
                return true;
            }
        }
    }
    return false;
}

bool World::tryMeltNeighbor(int x, int y) {
    if (randChance() >= 1) return false;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (!dx && !dy) continue;
            const int nx = x + dx, ny = y + dy;
            if (!isInside(nx, ny)) continue;

            ParticleInstance& np = at(nx, ny);
            if (np.id == ParticleRegistry::Empty) continue;

            const ParticlePhysics& nph = m_registry.physics(np.id);
            if ((nph.flags & ParticleFlags::CanMelt) && nph.meltInto != ParticleRegistry::Empty) {
                np.id = nph.meltInto;
                np.age = 0;
                np.brightness = randBrightness();
                wakeChunk(nx, ny);
                return true;
            }
        }
    }
    return false;
}

bool World::tryCorrode(int x, int y, ParticleInstance& p) {
    static constexpr int8_t offs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (const auto& o : offs) {
        const int nx = x + o[0], ny = y + o[1];
        if (!isInside(nx, ny)) continue;

        ParticleInstance& np = at(nx, ny);
        if (np.id != ParticleRegistry::Empty &&
            (m_registry.physics(np.id).flags & ParticleFlags::IsCorrodible)) {
            np.id = ParticleRegistry::Empty;
            np.age = 0;
            p.id = ParticleRegistry::Empty;
            p.age = 0;
            wakeChunk(nx, ny);
            wakeChunk(x, y);
            return true;
        }
    }
    return false;
}

void World::tryIgnite(int x, int y) {
    if (m_fireId == ParticleRegistry::Empty) return;
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            if (!dx && !dy) continue;
            const int nx = x + dx, ny = y + dy;
            if (!isInside(nx, ny)) continue;

            ParticleInstance& np = at(nx, ny);
            if (np.id != ParticleRegistry::Empty &&
                (m_registry.physics(np.id).flags & ParticleFlags::CanIgnite)) {
                np.id = m_fireId;
                np.age = 0;
                np.brightness = randBrightness();
                wakeChunk(nx, ny);
            }
        }
    }
}

void World::performSwap(const Vec2i& from, const Vec2i& to) {
    std::swap(at(from.x, from.y), at(to.x, to.y));
    setMoved(from.y * m_width + from.x);
    setMoved(to.y * m_width + to.x);
    wakeChunk(from.x, from.y);
    wakeChunk(to.x, to.y);
}

void World::setParticle(int x, int y, ParticleId id, uint8_t age) {
    if (!isInside(x, y)) return;
    ParticleInstance& p = at(x, y);
    p.id = id;
    p.age = age;
    p.brightness = (id != ParticleRegistry::Empty) ? randBrightness() : 0;
    wakeChunk(x, y);
}

void World::wakeChunk(int x, int y) {
    if (!isInside(x, y)) return;
    const int cx = x >> CHUNK_SHIFT;
    const int cy = y >> CHUNK_SHIFT;
    // Reset to 0 marks as FRESH source; neighbors are NOT woken here
    m_chunks[cy * m_chunksX + cx].idleFrames = 0;
}

void World::wakeNeighbors(int cx, int cy) {
    static constexpr int8_t offs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (const auto& o : offs) {
        const int nx = cx + o[0];
        const int ny = cy + o[1];
        if (nx >= 0 && nx < m_chunksX && ny >= 0 && ny < m_chunksY) {
            // Neighbors get idleFrames=1, NOT 0. This prevents them from becoming
            // new propagation sources in the same tick, stopping chain reactions.
            Chunk& c = m_chunks[ny * m_chunksX + nx];
            if (c.idleFrames > 1) c.idleFrames = 1;
        }
    }
}

bool World::isInside(int x, int y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}
