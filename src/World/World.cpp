#include "World/World.h"
#include <chrono>

static std::mt19937& getRng() {
    static std::mt19937 rng(
        std::chrono::steady_clock::now().time_since_epoch().count()
    );
    return rng;
}

inline void shuffle(Vec2i dirs[3], std::mt19937& rng) {
    int j = rng() % 3;
    if (j != 0) std::swap(dirs[0], dirs[j]);

    j = rng() % 2 + 1;
    if (j != 1) std::swap(dirs[1], dirs[j]);
}

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

    // Cache particle IDs
    m_smokeId = registry.findId("Smoke");
    m_fireId = registry.findId("Fire");

    // Cache particle definitions for zero-cost access
    for (int i = 0; i < 256; ++i) {
        m_defCache[i] = &registry.get(static_cast<ParticleId>(i));
    }
}

void World::loadParticles(const uint8_t* data, size_t size) {
    const size_t expectedSize = m_width * m_height * 2;

    if (size != expectedSize) {
        return;
    }

    const uint8_t* ptr = data;

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            ParticleId id = *ptr++;
            uint8_t age = *ptr++;

            ParticleInstance* p = getParticlePtr(x, y);
            p->id = id;
            p->age = age;
            p->brightness = (id != ParticleRegistry::Empty) ? (getRng()() % 256) : 0;
        }
    }

    for (int cy = 0; cy < m_chunksY; ++cy) {
        for (int cx = 0; cx < m_chunksX; ++cx) {
            int chunkIdx = cy * m_chunksX + cx;
            Chunk& chunk = m_chunks[chunkIdx];

            bool hasParticles = false;
            for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; ++i) {
                if (chunk.cells[i].id != ParticleRegistry::Empty) {
                    hasParticles = true;
                    break;
                }
            }

            if (hasParticles) {
                chunk.idleFrames = 0;
            } else {
                chunk.idleFrames = 31;
            }
        }
    }
}

// ====== TICK ======
void World::tick(float deltaTime) {
    m_accumulator += std::min(deltaTime, 0.05f);
    m_accumulator = std::min(m_accumulator, 0.1f);

    while (m_accumulator >= FIXED_DT) {
        int w = m_width;
        int total = w * m_height;

        std::fill(m_movedThisFrame.get(), m_movedThisFrame.get() + m_movedWords, 0);

        auto& rng = getRng();
        Vec2i dirs[5];

        static int frameCounter = 0;
        bool reverseOrder = (++frameCounter & 1);

        for (int cy = 0; cy < m_chunksY; ++cy) {
            int actualCy = reverseOrder ? (m_chunksY - 1 - cy) : cy;

            for (int cx = 0; cx < m_chunksX; ++cx) {
                int actualCx = reverseOrder ? (m_chunksX - 1 - cx) : cx;

                int chunkIdx = actualCy * m_chunksX + actualCx;
                Chunk& chunk = m_chunks[chunkIdx];

                if (chunk.idleFrames > 30) {
                    continue;
                }
                chunk.idleFrames++;

                int baseX = actualCx << 4;
                int baseY = actualCy << 4;
                int realMaxX = std::min(baseX + CHUNK_SIZE, m_width);
                int realMaxY = std::min(baseY + CHUNK_SIZE, m_height);

                for (int y = realMaxY - 1; y >= baseY; --y) {
                    int startX, endX, stepX;
                    bool rowParity = (y & 1) ^ reverseOrder;

                    if (rowParity) {
                        startX = baseX;
                        endX = realMaxX;
                        stepX = 1;
                    } else {
                        startX = realMaxX - 1;
                        endX = baseX - 1;
                        stepX = -1;
                    }

                    for (int x = startX; x != endX; x += stepX) {
                        int idx = y * w + x;
                        if (isMoved(idx)) continue;

                        ParticleInstance& p = at(x, y);
                        if (p.id == ParticleRegistry::Empty) {
                            p.age = 0;
                            continue;
                        }

                        p.age++;
                        const ParticleDefinition& def = *m_defCache[p.id];
                        int dx = m_distDir(rng);

                        switch (def.state) {
                            case PhysicalState::Powder: {
                                dirs[0] = {0, 1};
                                dirs[1] = {dx, 1};
                                dirs[2] = {-dx, 1};

                                for (int d = 0; d < 3; ++d) {
                                    Vec2i target(x + dirs[d].x, y + dirs[d].y);
                                    if (canMove({x, y}, target, def)) {
                                        performSwap({x, y}, target);
                                        break;
                                    }
                                }
                                break;
                            }

                            case PhysicalState::Liquid: {
                                dirs[0] = {0, 1};
                                dirs[1] = {dx, 1};
                                dirs[2] = {-dx, 1};
                                dirs[3] = {dx, 0};
                                dirs[4] = {-dx, 0};

                                for (int d = 0; d < 5; ++d) {
                                    Vec2i target(x + dirs[d].x, y + dirs[d].y);
                                    if (canMove({x, y}, target, def)) {
                                        performSwap({x, y}, target);
                                        break;
                                    }
                                }
                                break;
                            }

                            case PhysicalState::Gas: {
                                if (m_distChance(rng) < 5) {
                                    p.id = ParticleRegistry::Empty;
                                    break;
                                }

                                dirs[0] = {0, -1};
                                dirs[1] = {dx, -1};
                                dirs[2] = {-dx, -1};
                                dirs[3] = {dx, 0};
                                dirs[4] = {-dx, 0};

                                for (int d = 0; d < 5; ++d) {
                                    Vec2i target(x + dirs[d].x, y + dirs[d].y);
                                    if (canMove({x, y}, target, def)) {
                                        performSwap({x, y}, target);
                                        break;
                                    }
                                }
                                break;
                            }

                            case PhysicalState::Fire: {
                                if (p.age > 20 && m_distChance(rng) < 65 + p.age) {
                                    p.id = (m_smokeId != ParticleRegistry::Empty) ? m_smokeId : ParticleRegistry::Empty;
                                    p.age = 0;
                                    break;
                                }

                                dirs[0] = {0, -1};
                                dirs[1] = {dx, -1};
                                dirs[2] = {-dx, -1};
                                shuffle(dirs, rng);

                                for (int d = 0; d < 3; ++d) {
                                    Vec2i target(x + dirs[d].x, y + dirs[d].y);
                                    if (canMove({x, y}, target, def)) {
                                        performSwap({x, y}, target);
                                        break;
                                    }
                                }

                                if (m_distChance(rng) < 15) {
                                    for (int dy = -2; dy <= 2; ++dy) {
                                        for (int dx2 = -2; dx2 <= 2; ++dx2) {
                                            if (dx2 == 0 && dy == 0) continue;
                                            int nx = x + dx2;
                                            int ny = y + dy;
                                            if (isInside(nx, ny)) {
                                                ParticleId pid = at(nx, ny).id;
                                                if (pid != ParticleRegistry::Empty &&
                                                    m_defCache[pid]->canIgnite &&
                                                    m_fireId != ParticleRegistry::Empty) {
                                                    at(nx, ny).id = m_fireId;
                                                    at(nx, ny).age = 0;
                                                    wakeChunk(nx, ny);
                                                }
                                            }
                                        }
                                    }
                                }
                                break;
                            }

                            default: break;
                        }
                    }
                }
            }
        }

        m_accumulator -= FIXED_DT;
    }
}

// ====== WAKE CHUNK ======
void World::wakeChunk(int x, int y) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    int cx = x >> 4;
    int cy = y >> 4;
    int idx = cy * m_chunksX + cx;

    m_chunks[idx].idleFrames = 0;

    const int offsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (const auto& offset : offsets) {
        int ny = cy + offset[0];
        int nx = cx + offset[1];
        if (ny >= 0 && ny < m_chunksY && nx >= 0 && nx < m_chunksX) {
            int nIdx = ny * m_chunksX + nx;

             m_chunks[nIdx].idleFrames = 0;
        }
    }
}

// ====== CAN MOVE ======
inline bool World::canMove(const Vec2i& from, const Vec2i& to, const ParticleDefinition& fromDef) {
    if (!isInside(to.x, to.y)) return false;

    int toIdx = to.y * getWidth() + to.x;
    if (isMoved(toIdx)) return false;

    ParticleId toId = at(to.x, to.y).id;
    if (toId == ParticleRegistry::Empty) return true;

    const ParticleDefinition& toDef = *m_defCache[toId];

    if (toDef.state == PhysicalState::Solid) return false;

    return fromDef.density > toDef.density;
}

// ====== PERFORM SWAP ======
inline void World::performSwap(const Vec2i& from, const Vec2i& to) {
    std::swap(at(from.x, from.y), at(to.x, to.y));

    int fromIdx = from.y * getWidth() + from.x;
    int toIdx = to.y * getWidth() + to.x;
    setMoved(toIdx);
    setMoved(fromIdx);

    wakeChunk(from.x, from.y);
    wakeChunk(to.x, to.y);
}

// ====== SET PARTICLE ======
void World::setParticle(int x, int y, ParticleId id, uint8_t age) {
    if (!isInside(x, y)) return;
    auto& p = at(x, y);
    p.id = id;

    p.age = age;

    if (id != ParticleRegistry::Empty) p.brightness = getRng()() % 256;

    wakeChunk(x, y);
}

// ====== IS INSIDE ======
bool World::isInside(int x, int y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}
