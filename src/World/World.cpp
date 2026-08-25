#include "World/World.h"
#include <cstring>
#include <chrono>

static std::mt19937& getRng() {
    static std::mt19937 rng(
        std::chrono::steady_clock::now().time_since_epoch().count()
    );
    return rng;
}

// ====== CONSTRUCTOR ======
World::World(int width, int height, ParticleRegistry& registry)
    : m_width(width)
    , m_height(height)
    , m_registry(registry) {

    m_chunksX = (width + CHUNK_SIZE - 1) / CHUNK_SIZE;
    m_chunksY = (height + CHUNK_SIZE - 1) / CHUNK_SIZE;

    m_chunks = std::make_unique<Chunk[]>(m_chunksX * m_chunksY);
    m_movedThisFrame = std::make_unique<bool[]>(width * height);
    m_chunkInQueue.resize(m_chunksX * m_chunksY, 0);

    // Cache particle IDs
    m_smokeId = registry.findId("Smoke");
    m_fireId = registry.findId("Fire");

    // Cache particle definitions for zero-cost access
    for (int i = 0; i < 256; ++i) {
        m_defCache[i] = &registry.get(static_cast<ParticleId>(i));
    }
}

// ====== INIT ACTIVE CHUNKS ======
void World::initActiveChunks() {
    for (int cy = 0; cy < m_chunksY; ++cy) {
        for (int cx = 0; cx < m_chunksX; ++cx) {
            Chunk& chunk = m_chunks[cy * m_chunksX + cx];
            bool hasParticles = false;

            for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; ++i) {
                if (chunk.cells[i].id != ParticleRegistry::Empty) {
                    hasParticles = true;
                    break;
                }
            }

            if (hasParticles) {
                int idx = cy * m_chunksX + cx;
                m_chunkInQueue[idx] = 1;
                m_activeChunks.push_back(idx);
                chunk.idleFrames = 0;
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

        std::fill(m_movedThisFrame.get(), m_movedThisFrame.get() + total, false);

        auto& rng = getRng();
        std::uniform_int_distribution<int> distDir(-1, 1);
        std::uniform_int_distribution<int> distChance(0, 99);

        // Pre-allocate direction arrays (reused for all particles)
        Vec2i dirs[5];

        for (size_t i = 0; i < m_activeChunks.size(); ++i) {
            int chunkIdx = m_activeChunks[i];
            Chunk& chunk = m_chunks[chunkIdx];

            if (chunk.idleFrames > 30) {
                chunk.idleFrames = 0;
                m_chunkInQueue[chunkIdx] = 0;
                m_activeChunks[i] = m_activeChunks.back();
                m_activeChunks.pop_back();
                --i;
                continue;
            }

            chunk.idleFrames++;

            int cx = chunkIdx % m_chunksX;
            int cy = chunkIdx / m_chunksX;
            int baseX = cx << 4;
            int baseY = cy << 4;
            int maxX = std::min(baseX + CHUNK_SIZE, m_width);
            int maxY = std::min(baseY + CHUNK_SIZE, m_height);

            for (int y = maxY - 1; y >= baseY; --y) {
                int startX = (y & 1) ? baseX : maxX - 1;
                int endX = (y & 1) ? maxX : baseX - 1;
                int stepX = (y & 1) ? 1 : -1;

                for (int x = startX; x != endX; x += stepX) {
                    int idx = y * w + x;
                    if (m_movedThisFrame[idx]) continue;

                    ParticleInstance& p = at(x, y);
                    if (p.id == ParticleRegistry::Empty) {
                        p.age = 0;
                        continue;
                    }

                    p.age++;
                    const ParticleDefinition& def = *m_defCache[p.id];
                    int dx = distDir(rng);

                    switch (def.state) {
                        case PhysicalState::Powder: {
                            dirs[0] = {0, 1};
                            dirs[1] = {dx, 1};
                            dirs[2] = {-dx, 1};

                            for (int d = 0; d < 3; ++d) {
                                Vec2i target(x + dirs[d].x, y + dirs[d].y);
                                if (canMove({x, y}, target, def)) {
                                    performSwap({x, y}, target);
                                    goto next_particle;
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
                                    goto next_particle;
                                }
                            }
                            break;
                        }

                        case PhysicalState::Gas: {
                            if (distChance(rng) < 5) {
                                p.id = ParticleRegistry::Empty;
                                goto next_particle;
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
                                    goto next_particle;
                                }
                            }
                            break;
                        }

                        case PhysicalState::Fire: {
                            if (p.age > 20 && distChance(rng) < 65 + p.age) {
                                p.id = (m_smokeId != ParticleRegistry::Empty) ? m_smokeId : ParticleRegistry::Empty;
                                p.age = 0;
                                goto next_particle;
                            }

                            dirs[0] = {0, -1};
                            dirs[1] = {dx, -1};
                            dirs[2] = {-dx, -1};

                            // Shuffle first 3 directions
                            for (int d = 2; d > 0; --d) {
                                int j = std::uniform_int_distribution<>(0, d)(rng);
                                std::swap(dirs[d], dirs[j]);
                            }

                            for (int d = 0; d < 3; ++d) {
                                Vec2i target(x + dirs[d].x, y + dirs[d].y);
                                if (canMove({x, y}, target, def)) {
                                    performSwap({x, y}, target);
                                    goto next_particle;
                                }
                            }

                            // Ignite nearby flammable particles
                            if (distChance(rng) < 15) {
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

                    next_particle:;
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

    if (!m_chunkInQueue[idx]) {
        m_chunkInQueue[idx] = 1;
        m_chunks[idx].idleFrames = 0;
        m_activeChunks.push_back(idx);
    } else {
        m_chunks[idx].idleFrames = 0;
    }

    // Wake neighbors (macro for brevity)
    #define WAKE_NEIGHBOR(ny, nx) \
        if ((ny) >= 0 && (ny) < m_chunksY && (nx) >= 0 && (nx) < m_chunksX) { \
            int nIdx = (ny) * m_chunksX + (nx); \
            if (!m_chunkInQueue[nIdx]) { \
                m_chunkInQueue[nIdx] = 1; \
                m_chunks[nIdx].idleFrames = 0; \
                m_activeChunks.push_back(nIdx); \
            } else { \
                m_chunks[nIdx].idleFrames = 0; \
            } \
        }

    WAKE_NEIGHBOR(cy - 1, cx);
    WAKE_NEIGHBOR(cy + 1, cx);
    WAKE_NEIGHBOR(cy, cx - 1);
    WAKE_NEIGHBOR(cy, cx + 1);

    #undef WAKE_NEIGHBOR
}

// ====== CAN MOVE ======
inline bool World::canMove(const Vec2i& from, const Vec2i& to, const ParticleDefinition& fromDef) {
    if (!isInside(to.x, to.y)) return false;

    int toIdx = to.y * getWidth() + to.x;
    if (m_movedThisFrame[toIdx]) return false;

    ParticleId toId = at(to.x, to.y).id;
    if (toId == ParticleRegistry::Empty) return true;

    const ParticleDefinition& toDef = *m_defCache[toId];
    return toDef.state != PhysicalState::Solid && fromDef.density > toDef.density;
}

// ====== PERFORM SWAP ======
inline void World::performSwap(const Vec2i& from, const Vec2i& to) {
    std::swap(at(from.x, from.y), at(to.x, to.y));

    int fromIdx = from.y * getWidth() + from.x;
    int toIdx = to.y * getWidth() + to.x;
    m_movedThisFrame[toIdx] = 1;
    m_movedThisFrame[fromIdx] = 1;

    wakeChunk(from.x, from.y);
    wakeChunk(to.x, to.y);
}

// ====== SET PARTICLE ======
void World::setParticle(int x, int y, ParticleId id) {
    if (!isInside(x, y)) return;
    auto& p = at(x, y);
    p.id = id;
    p.age = 0;
    wakeChunk(x, y);
}

// ====== IS INSIDE ======
bool World::isInside(int x, int y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}
