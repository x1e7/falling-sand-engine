#include "World/World.h"

#include <algorithm>

World::World(int width, int height, ParticleRegistry& registry)
    : m_width(width)
    , m_height(height)
    , m_registry(registry) {
    m_chunksX = (width + CHUNK_SIZE - 1) >> CHUNK_SHIFT;
    m_chunksY = (height + CHUNK_SIZE - 1) >> CHUNK_SHIFT;
    m_chunks = std::make_unique<Chunk[]>(static_cast<size_t>(m_chunksX) * m_chunksY);

    m_movedWords = (width * height + 31) / 32;
    m_movedThisFrame = std::make_unique<uint32_t[]>(static_cast<size_t>(m_movedWords));

    m_smokeId = registry.findId("Smoke");
    m_fireId  = registry.findId("Fire");
}

void World::loadParticles(const uint8_t* data, size_t size) {
    if (size != static_cast<size_t>(m_width) * m_height * 2) return;

    const uint8_t* ptr = data;
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            ParticleInstance& p = at(x, y);
            p.id = *ptr++;
            p.age = *ptr++;
            p.vx = 0;
            p.vy = 0;
            p.brightness = (p.id != ParticleRegistry::Empty) ? randBrightness() : 0;
        }
    }

    for (int cy = 0; cy < m_chunksY; ++cy) {
        for (int cx = 0; cx < m_chunksX; ++cx) {
            Chunk& chunk = m_chunks[cy * m_chunksX + cx];
            chunk.idleFrames = IDLE_THRESHOLD + 1;
            for (int i = 0; i < CHUNK_AREA; ++i) {
                if (chunk.cells[i].id != 0) {
                    chunk.idleFrames = 0;
                    break;
                }
            }
        }
    }
}

void World::tick(float deltaTime) {
    m_accumulator = std::min(m_accumulator + deltaTime, 0.1f);
    m_ticksLastFrame = 0;
    m_activeChunks = 0;

    while (m_accumulator >= FIXED_DT) {
        const bool reverse = (++m_frameCounter & 1) != 0;
        std::fill(m_movedThisFrame.get(), m_movedThisFrame.get() + m_movedWords, 0u);

        for (int cy = 0; cy < m_chunksY; ++cy) {
            int ay = reverse ? m_chunksY - 1 - cy : cy;
            for (int cx = 0; cx < m_chunksX; ++cx) {
                int ax = reverse ? m_chunksX - 1 - cx : cx;
                Chunk& chunk = m_chunks[ay * m_chunksX + ax];
                if (chunk.idleFrames > IDLE_THRESHOLD) continue;

                chunk.idleFrames++;
                m_activeChunks++;

                int baseX = ax << CHUNK_SHIFT;
                int baseY = ay << CHUNK_SHIFT;
                int maxX = std::min(baseX + CHUNK_SIZE, m_width);
                int maxY = std::min(baseY + CHUNK_SIZE, m_height);

                for (int y = maxY - 1; y >= baseY; --y) {
                    bool rowParity = ((y & 1) != 0) ^ reverse;
                    int startX = rowParity ? baseX : maxX - 1;
                    int endX   = rowParity ? maxX  : baseX - 1;
                    int stepX  = rowParity ? 1 : -1;
                    for (int x = startX; x != endX; x += stepX) {
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

    if (ph.flags & ParticleFlags::CanMelt) {
        if (tryMeltSelf(x, y, p, ph)) return;
    }

    switch (ph.state) {
        case PhysicalState::Powder: updatePowder(x, y, p); break;
        case PhysicalState::Liquid: updateLiquid(x, y, p, ph); break;
        case PhysicalState::Gas:    updateGas(x, y, p); break;
        case PhysicalState::Fire:   updateFire(x, y, p); break;
        default: return;
    }

    if (p.id == ParticleRegistry::Empty) return;
    if (p.vx == 0 && p.vy == 0) return;

    tryMoveInDirection(x, y, velocityToDirection(p.vx, p.vy), p.id);
}

void World::updatePowder(int x, int y, ParticleInstance& p) {
    int dx = randDir();
    Vec2i dirs[3] = {{0, 1}, {dx, 1}, {-dx, 1}};
    updateVelocity(x, y, dirs, 3, p);
}

void World::updateLiquid(int x, int y, ParticleInstance& p, const ParticlePhysics& ph) {
    if (ph.flags & ParticleFlags::IsCorrosive) {
        if (tryCorrode(x, y, p)) return;
    }
    if (ph.flags & ParticleFlags::IsHot) {
        if (tryMeltNeighbor(x, y)) return;
    }

    int dx = randDir();
    Vec2i dirs[5] = {{0, 1}, {dx, 1}, {-dx, 1}, {dx, 0}, {-dx, 0}};
    updateVelocity(x, y, dirs, 5, p);
}

void World::updateGas(int x, int y, ParticleInstance& p) {
    if (randChance() < 5) {
        p.id = ParticleRegistry::Empty;
        p.age = 0;
        wakeChunk(x, y);
        return;
    }

    int dx = randDir();
    Vec2i dirs[5] = {{0, -1}, {dx, -1}, {-dx, -1}, {dx, 0}, {-dx, 0}};
    updateVelocity(x, y, dirs, 5, p);
}

void World::updateFire(int x, int y, ParticleInstance& p) {
    if (p.age > 20 && randChance() < 65 + static_cast<int>(p.age)) {
        p.id = (m_smokeId != ParticleRegistry::Empty) ? m_smokeId : ParticleRegistry::Empty;
        p.age = 0;
        p.vx = 0;
        p.vy = 0;
        wakeChunk(x, y);
        return;
    }

    int dx = randDir();
    Vec2i dirs[3] = {{0, -1}, {dx, -1}, {-dx, -1}};
    updateVelocity(x, y, dirs, 3, p);

    if (randChance() < 15) tryIgnite(x, y);
}

bool World::tryMeltSelf(int x, int y, ParticleInstance& p, const ParticlePhysics& ph) {
    if (ph.meltInto == ParticleRegistry::Empty) return false;
    if (randChance() >= 10) return false;

    for (int dy = -1; dy <= 0; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (!dx && !dy) continue;
            int nx = x + dx, ny = y + dy;
            if (!isInside(nx, ny)) continue;
            if (m_registry.physics(at(nx, ny).id).flags & ParticleFlags::IsHot) {
                p.id = ph.meltInto;
                p.age = 0;
                p.vx = 0;
                p.vy = 0;
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
            int nx = x + dx, ny = y + dy;
            if (!isInside(nx, ny)) continue;

            ParticleInstance& np = at(nx, ny);
            if (np.id == ParticleRegistry::Empty) continue;

            const ParticlePhysics& nph = m_registry.physics(np.id);
            if (!(nph.flags & ParticleFlags::CanMelt)) continue;
            if (nph.meltInto == ParticleRegistry::Empty) continue;

            np.id = nph.meltInto;
            np.age = 0;
            np.vx = 0;
            np.vy = 0;
            np.brightness = randBrightness();
            wakeChunk(nx, ny);
            return true;
        }
    }
    return false;
}

bool World::tryCorrode(int x, int y, ParticleInstance& p) {
    static constexpr int8_t offs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (const auto& o : offs) {
        int nx = x + o[0], ny = y + o[1];
        if (!isInside(nx, ny)) continue;

        ParticleInstance& np = at(nx, ny);
        if (np.id == ParticleRegistry::Empty) continue;
        if (!(m_registry.physics(np.id).flags & ParticleFlags::IsCorrodible)) continue;

        np.id = ParticleRegistry::Empty;
        np.age = 0;
        p.id = ParticleRegistry::Empty;
        p.age = 0;
        wakeChunk(nx, ny);
        wakeChunk(x, y);
        return true;
    }
    return false;
}

void World::tryIgnite(int x, int y) {
    if (m_fireId == ParticleRegistry::Empty) return;

    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            if (!dx && !dy) continue;
            int nx = x + dx, ny = y + dy;
            if (!isInside(nx, ny)) continue;

            ParticleInstance& np = at(nx, ny);
            if (np.id == ParticleRegistry::Empty) continue;
            if (!(m_registry.physics(np.id).flags & ParticleFlags::CanIgnite)) continue;

            np.id = m_fireId;
            np.age = 0;
            np.vx = 0;
            np.vy = 0;
            wakeChunk(nx, ny);
        }
    }
}

void World::updateVelocity(int x, int y, const Vec2i* dirs, int count, ParticleInstance& p) {
    const ParticleId fromId = p.id;
    for (int i = 0; i < count; ++i) {
        Vec2i target{x + dirs[i].x, y + dirs[i].y};
        if (canMove(target, fromId)) {
            p.vx = static_cast<int8_t>(dirs[i].x);
            p.vy = static_cast<int8_t>(dirs[i].y);
            return;
        }
    }
    p.vx = 0;
    p.vy = 0;
}

bool World::tryMoveInDirection(int x, int y, Vec2i dir, ParticleId fromId) {
    Vec2i dirs[3] = {
        {dir.x, dir.y},
        {dir.x, 0},
        {0, dir.y}
    };

    for (const auto& d : dirs) {
        if (d.x == 0 && d.y == 0) continue;
        Vec2i target{x + d.x, y + d.y};
        if (canMove(target, fromId)) {
            performSwap({x, y}, target);
            return true;
        }
    }
    return false;
}

bool World::canMove(const Vec2i& to, ParticleId fromId) {
    if (!isInside(to.x, to.y)) return false;
    if (isMoved(to.y * m_width + to.x)) return false;
    return m_registry.canDisplace(fromId, at(to.x, to.y).id);
}

void World::performSwap(const Vec2i& from, const Vec2i& to) {
    std::swap(at(from.x, from.y), at(to.x, to.y));
    setMoved(from.y * m_width + from.x);
    setMoved(to.y * m_width + to.x);
    wakeChunk(from.x, from.y);
    wakeChunk(to.x, to.y);
}

void World::wakeChunk(int x, int y) {
    if (!isInside(x, y)) return;

    int cx = x >> CHUNK_SHIFT;
    int cy = y >> CHUNK_SHIFT;
    m_chunks[cy * m_chunksX + cx].idleFrames = 0;

    static constexpr int8_t offs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (const auto& o : offs) {
        int nx = cx + o[0], ny = cy + o[1];
        if (nx >= 0 && nx < m_chunksX && ny >= 0 && ny < m_chunksY) {
            m_chunks[ny * m_chunksX + nx].idleFrames = 0;
        }
    }
}

void World::setParticle(int x, int y, ParticleId id, uint8_t age) {
    if (!isInside(x, y)) return;

    ParticleInstance& p = at(x, y);
    p.id = id;
    p.age = age;
    p.vx = 0;
    p.vy = 0;
    p.brightness = (id != ParticleRegistry::Empty) ? randBrightness() : 0;
    wakeChunk(x, y);
}

bool World::isInside(int x, int y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}
