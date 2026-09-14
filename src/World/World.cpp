#include "World/World.h"
#include <algorithm>

// ====== CONSTRUCTOR ======
World::World(int width, int height, ParticleRegistry& registry)
    : m_width(width)
    , m_height(height)
    , m_registry(registry) {

    m_chunksX = (width + CHUNK_SIZE - 1) / CHUNK_SIZE;
    m_chunksY = (height + CHUNK_SIZE - 1) / CHUNK_SIZE;

    m_chunks = std::make_unique<Chunk[]>(m_chunksX * m_chunksY);
    m_movedWords = (width * height + 31) / 32;
    m_movedThisFrame = std::make_unique<uint32_t[]>(m_movedWords);

    m_smokeId = registry.findId("Smoke");
    m_fireId  = registry.findId("Fire");
}

// ====== LOAD ======
void World::loadParticles(const uint8_t* data, size_t size) {
    if (size != static_cast<size_t>(m_width) * m_height * 2) return;

    const uint8_t* ptr = data;
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            ParticleInstance* p = getParticlePtr(x, y);
            p->id = *ptr++;
            p->age = *ptr++;
            p->brightness = (p->id != ParticleRegistry::Empty) ? (m_rng() % 256) : 0;
        }
    }

    for (int cy = 0; cy < m_chunksY; ++cy) {
        for (int cx = 0; cx < m_chunksX; ++cx) {
            Chunk& chunk = m_chunks[cy * m_chunksX + cx];
            chunk.idleFrames = 31;
            for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; ++i) {
                if (chunk.cells[i].id != ParticleRegistry::Empty) {
                    chunk.idleFrames = 0;
                    break;
                }
            }
        }
    }
}

// ====== TICK ======
void World::tick(float deltaTime) {
    m_accumulator = std::min(m_accumulator + deltaTime, 0.1f);

    const bool reverse = (++m_frameCounter & 1) != 0;

    while (m_accumulator >= FIXED_DT) {
        std::fill(m_movedThisFrame.get(), m_movedThisFrame.get() + m_movedWords, 0);

        for (int cy = 0; cy < m_chunksY; ++cy) {
            int ay = reverse ? m_chunksY - 1 - cy : cy;
            for (int cx = 0; cx < m_chunksX; ++cx) {
                int ax = reverse ? m_chunksX - 1 - cx : cx;

                Chunk& chunk = m_chunks[ay * m_chunksX + ax];
                if (chunk.idleFrames > 30) continue;
                chunk.idleFrames++;

                int baseX = ax << 4;
                int baseY = ay << 4;
                int maxX = std::min(baseX + CHUNK_SIZE, m_width);
                int maxY = std::min(baseY + CHUNK_SIZE, m_height);

                for (int y = maxY - 1; y >= baseY; --y) {
                    bool rowParity = (y & 1) ^ reverse;
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
    }
}

// ====== CELL UPDATE ======
void World::updateCell(int x, int y) {
    int idx = y * m_width + x;
    if (isMoved(idx)) return;

    ParticleInstance& p = at(x, y);
    if (p.id == ParticleRegistry::Empty) {
        p.age = 0;
        return;
    }

    p.age++;
    const ParticleDefinition& def = m_registry.get(p.id);

    if (def.canMelt) tryMeltSelf(x, y, p, def);
    if (p.id == ParticleRegistry::Empty) return;

    const ParticleDefinition& curDef = m_registry.get(p.id);

    switch (curDef.state) {
        case PhysicalState::Powder: updatePowder(x, y, p, curDef); break;
        case PhysicalState::Liquid: updateLiquid(x, y, p, curDef); break;
        case PhysicalState::Gas:    updateGas   (x, y, p, curDef); break;
        case PhysicalState::Fire:   updateFire  (x, y, p, curDef); break;
        default: break;
    }
}

// ====== BEHAVIORS ======
void World::updatePowder(int x, int y, ParticleInstance&, const ParticleDefinition& def) {
    int dx = m_distDir(m_rng);
    Vec2i dirs[3] = {{0, 1}, {dx, 1}, {-dx, 1}};
    tryMove(x, y, dirs, 3, def);
}

void World::updateLiquid(int x, int y, ParticleInstance& p, const ParticleDefinition& def) {
    if (def.isCorrosive) tryCorrode(x, y, p);
    if (p.id == ParticleRegistry::Empty) return;

    if (def.isHot) tryMeltNeighbor(x, y, p);

    int dx = m_distDir(m_rng);
    Vec2i dirs[5] = {{0, 1}, {dx, 1}, {-dx, 1}, {dx, 0}, {-dx, 0}};
    tryMove(x, y, dirs, 5, def);
}

void World::updateGas(int x, int y, ParticleInstance& p, const ParticleDefinition& def) {
    if (m_distChance(m_rng) < 5) {
        p.id = ParticleRegistry::Empty;
        return;
    }

    int dx = m_distDir(m_rng);
    Vec2i dirs[5] = {{0, -1}, {dx, -1}, {-dx, -1}, {dx, 0}, {-dx, 0}};
    tryMove(x, y, dirs, 5, def);
}

void World::updateFire(int x, int y, ParticleInstance& p, const ParticleDefinition& def) {
    if (p.age > 20 && m_distChance(m_rng) < 65 + p.age) {
        p.id = (m_smokeId != ParticleRegistry::Empty) ? m_smokeId : ParticleRegistry::Empty;
        p.age = 0;
        wakeChunk(x, y);
        return;
    }

    int dx = m_distDir(m_rng);
    Vec2i dirs[3] = {{0, -1}, {dx, -1}, {-dx, -1}};
    tryMove(x, y, dirs, 3, def);

    if (m_distChance(m_rng) < 15) tryIgnite(x, y);
}

void World::tryMeltSelf(int x, int y, ParticleInstance& p, const ParticleDefinition& def) {
    if (def.meltInto == ParticleRegistry::Empty) return;
    if (m_distChance(m_rng) >= 10) return;

    for (int dy = -1; dy <= 0; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (!dx && !dy) continue;
            int nx = x + dx, ny = y + dy;
            if (!isInside(nx, ny)) continue;
            if (m_registry.get(at(nx, ny).id).isHot) {
                p.id = def.meltInto;
                p.age = 0;
                p.brightness = m_rng() % 256;
                wakeChunk(x, y);
                return;
            }
        }
    }
}

void World::tryMeltNeighbor(int x, int y, ParticleInstance& p) {
    if (p.age < 255) return;
    if (m_distChance(m_rng) >= 1) return;

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (!dx && !dy) continue;
            int nx = x + dx, ny = y + dy;
            if (!isInside(nx, ny)) continue;

            ParticleInstance& np = at(nx, ny);
            if (np.id == ParticleRegistry::Empty) continue;

            const ParticleDefinition& ndef = m_registry.get(np.id);
            if (ndef.meltInto == ParticleRegistry::Empty) continue;

            np.id = ndef.meltInto;
            np.age = 0;
            np.brightness = m_rng() % 256;
            wakeChunk(nx, ny);
            return;
        }
    }
}

void World::tryCorrode(int x, int y, ParticleInstance& p) {
    static constexpr int8_t offs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (auto& o : offs) {
        int nx = x + o[0], ny = y + o[1];
        if (!isInside(nx, ny)) continue;

        ParticleInstance& np = at(nx, ny);
        if (np.id == ParticleRegistry::Empty) continue;
        if (!m_registry.get(np.id).isCorrodible) continue;

        np.id = ParticleRegistry::Empty;
        p.id = ParticleRegistry::Empty;
        wakeChunk(nx, ny);
        return;
    }
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
            if (!m_registry.get(np.id).canIgnite) continue;

            np.id = m_fireId;
            np.age = 0;
            wakeChunk(nx, ny);
        }
    }
}

bool World::tryMove(int x, int y, const Vec2i* dirs, int count, const ParticleDefinition& def) {
    for (int i = 0; i < count; ++i) {
        Vec2i target{x + dirs[i].x, y + dirs[i].y};
        if (canMove(target, def)) {
            performSwap({x, y}, target);
            return true;
        }
    }
    return false;
}

bool World::canMove(const Vec2i& to, const ParticleDefinition& fromDef) {
    if (!isInside(to.x, to.y)) return false;
    if (isMoved(to.y * m_width + to.x)) return false;

    ParticleId toId = at(to.x, to.y).id;
    if (toId == ParticleRegistry::Empty) return true;

    const ParticleDefinition& toDef = m_registry.get(toId);

    if (toDef.state == PhysicalState::Solid) return false;
    if (fromDef.state == PhysicalState::Gas) return true;
    if (toDef.state == PhysicalState::Gas) return true;

    if (fromDef.state == PhysicalState::Liquid && toDef.state == PhysicalState::Powder)
        return false;

    if (fromDef.state == PhysicalState::Powder && toDef.state == PhysicalState::Liquid)
        return true;

    return fromDef.density > toDef.density;
}

void World::performSwap(const Vec2i& from, const Vec2i& to) {
    std::swap(at(from.x, from.y), at(to.x, to.y));
    setMoved(from.y * m_width + from.x);
    setMoved(to.y * m_width + to.x);
    wakeChunk(from.x, from.y);
    wakeChunk(to.x, to.y);
}

void World::wakeChunk(int x, int y) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    int cx = x >> 4, cy = y >> 4;
    int idx = cy * m_chunksX + cx;
    m_chunks[idx].idleFrames = 0;

    static constexpr int8_t offs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (auto& o : offs) {
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
    if (id != ParticleRegistry::Empty) p.brightness = m_rng() % 256;
    wakeChunk(x, y);
}

bool World::isInside(int x, int y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}
