#include "World/World.h"
#include <cstring>
#include <random>
#include <chrono>
#include <cmath>

static std::mt19937& getRng() {
    static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    return rng;
}

World::World(int width, int height, ParticleRegistry& registry)
    : m_width(width), m_height(height), m_registry(registry) {

    m_chunksX = (width + CHUNK_SIZE - 1) / CHUNK_SIZE;
    m_chunksY = (height + CHUNK_SIZE - 1) / CHUNK_SIZE;

    m_chunks = new Chunk[m_chunksX * m_chunksY];

    m_movedThisFrame = new bool[width * height];
    std::memset(m_movedThisFrame, 0, width * height * sizeof(bool));
}

World::~World() {
    delete[] m_chunks;
    delete[] m_movedThisFrame;
}

void World::setParticle(int x, int y, ParticleId id) {
    if (!isInside(x, y)) return;
    auto& p = at(x, y);
    p.id = id;
    p.age = 0;

    wakeChunk(x, y);
}

bool World::isInside(int x, int y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

void World::tick(float deltaTime) {
    m_accumulator += std::min(deltaTime, 0.05f);
    m_accumulator = std::min(m_accumulator, 0.1f);

    while (m_accumulator >= FIXED_DT) {
        int w = m_width, h = m_height;
        int total = w * h;

        std::memset(m_movedThisFrame, 0, total * sizeof(bool));

        for (int cy = 0; cy < m_chunksY; ++cy) {
            for (int cx = 0; cx < m_chunksX; ++cx) {
                Chunk& chunk = m_chunks[cy * m_chunksX + cx];

                if (chunk.idleFrames > 30) continue;

                chunk.idleFrames++;

                int baseX = cx << 4;
                int baseY = cy << 4;
                int maxX = std::min(baseX + CHUNK_SIZE, m_width);
                int maxY = std::min(baseY + CHUNK_SIZE, m_height);

                for (int y = maxY - 1; y >= baseY; --y) {
                    int startX = (y % 2 == 0) ? baseX : maxX - 1;
                    int endX = (y % 2 == 0) ? maxX : baseX - 1;
                    int stepX = (y % 2 == 0) ? 1 : -1;

                    for (int x = startX; x != endX; x += stepX) {
                        if (m_movedThisFrame[y * w + x]) continue;

                        ParticleInstance& p = at(x, y);
                        if (p.id == ParticleRegistry::Empty) {
                            p.age = 0;
                            continue;
                        }

                        p.age++;

                        const auto& def = m_registry.get(p.id);
                        switch (def.state) {
                            case PhysicalState::Powder: updatePowder({x,y}); break;
                            case PhysicalState::Liquid:  updateLiquid({x,y}); break;
                            case PhysicalState::Gas:    updateGas({x,y}); break;
                            case PhysicalState::Fire:   updateFire({x,y}); break;
                            default: break;
                        }
                    }
                }
            }
        }
        m_accumulator -= FIXED_DT;
    }
}

void World::wakeChunk(int x, int y) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    int cx = x >> 4;
    int cy = y >> 4;

    Chunk& chunk = m_chunks[cy * m_chunksX + cx];
    chunk.idleFrames = 0;

    if (cy > 0) {
        Chunk& above = m_chunks[(cy - 1) * m_chunksX + cx];
        above.idleFrames = 0;
    }
    if (cy + 1 < m_chunksY) {
        Chunk& below = m_chunks[(cy + 1) * m_chunksX + cx];
        below.idleFrames = 0;
    }
    if (cx > 0) {
        Chunk& left = m_chunks[cy * m_chunksX + (cx - 1)];
        left.idleFrames = 0;
    }
    if (cx + 1 < m_chunksX) {
        Chunk& right = m_chunks[cy * m_chunksX + (cx + 1)];
        right.idleFrames = 0;
    }
}
void World::updatePowder(const Vec2i& pos) {
    auto& rng = getRng();
    int dx = std::uniform_int_distribution<>(-1, 1)(rng);

    Vec2i dirs[] = {{0, 1}, {dx, 1}, {-dx, 1}};

    for (auto& dir : dirs) {
        Vec2i target(pos.x + dir.x, pos.y + dir.y);
        if (canMove(pos, target)) {
            performSwap(pos, target);
            return;
        }
    }
}

void World::updateLiquid(const Vec2i& pos) {
    auto& rng = getRng();

    int dx = std::uniform_int_distribution<>(-1, 1)(rng);

    // Build direction list with priority
    Vec2i dirs[] = {
        {0, 1},
        {dx, 1},
        {-dx, 1},
        {dx, 0},
        {-dx, 0}
    };

    for (auto& dir : dirs) {
        Vec2i target(pos.x + dir.x, pos.y + dir.y);
        if (canMove(pos, target)) {
            performSwap(pos, target);
            return;
        }
    }
}

void World::updateGas(const Vec2i& pos) {
    auto& rng = getRng();
    ParticleInstance& p = at(pos.y, pos.x);

    if (std::uniform_int_distribution<>(0, 99)(rng) < 5) {
        setParticle(pos.x, pos.y, ParticleRegistry::Empty);
        return;
    }

    int dx = std::uniform_int_distribution<>(-1, 1)(rng);

    Vec2i dirs[] = {
        {0, -1},
        {dx, -1},
        {-dx, -1},
        {dx, 0},
        {-dx, 0}
    };

    for (auto& dir : dirs) {
        Vec2i target(pos.x + dir.x, pos.y + dir.y);
        if (canMove(pos, target)) {
            performSwap(pos, target);
            return;
        }
    }
}

void World::updateFire(const Vec2i& pos) {
    auto& rng = getRng();

    if (!isInside(pos.x, pos.y)) return;

    int dx = std::uniform_int_distribution<>(-1, 1)(rng);
    int age = at(pos.x, pos.y).age;

    if (age > 20 && std::uniform_int_distribution<>(0, 99)(rng) < 65 + age) {
        ParticleId smokeId = getRegistry().findId("Smoke");
        setParticle(pos.x, pos.y, smokeId != ParticleRegistry::Empty ? smokeId : ParticleRegistry::Empty);
        return;
    }

    Vec2i dirs[] = {{0, -1}, {dx, -1}, {-dx, -1}};
    for (int i = 2; i > 0; --i) {
        int j = std::uniform_int_distribution<>(0, i)(rng);
        std::swap(dirs[i], dirs[j]);
    }

    for (auto& dir : dirs) {
        Vec2i target(pos.x + dir.x, pos.y + dir.y);
        if (canMove(pos, target)) {
            performSwap(pos, target);
            return;
        }
    }

    if (std::uniform_int_distribution<>(0, 99)(rng) < 15) {
        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                if (dx == 0 && dy == 0) continue;
                Vec2i n(pos.x + dx, pos.y + dy);
                if (isInside(n.x, n.y)) {
                    ParticleId pid = at(n.x, n.y).id;
                    if (pid != ParticleRegistry::Empty && getRegistry().get(pid).canIgnite) {
                        ParticleId fireId = getRegistry().findId("Fire");
                        if (fireId != ParticleRegistry::Empty) {
                            setParticle(n.x, n.y, fireId);
                        }
                    }
                }
            }
        }
    }
}

bool World::canMove(const Vec2i& from, const Vec2i& to) {
    if (!isInside(to.x, to.y)) return false;

    int toIdx = to.y * getWidth() + to.x;
    if (m_movedThisFrame[toIdx]) return false;

    ParticleId fromId = at(from.x, from.y).id;
    ParticleId toId = at(to.x, to.y).id;

    if (toId == ParticleRegistry::Empty) return true;

    const auto& fromDef = getRegistry().get(fromId);
    const auto& toDef = getRegistry().get(toId);

    return toDef.state != PhysicalState::Solid && fromDef.density > toDef.density;
}

void World::performSwap(const Vec2i& from, const Vec2i& to) {
    std::swap(at(from.x, from.y), at(to.x, to.y));

    int fromIdx = from.y * getWidth() + from.x;
    int toIdx = to.y * getWidth() + to.x;
    m_movedThisFrame[toIdx] = m_movedThisFrame[fromIdx] = 1;

    wakeChunk(from.x, from.y);
    wakeChunk(to.x, to.y);
}
