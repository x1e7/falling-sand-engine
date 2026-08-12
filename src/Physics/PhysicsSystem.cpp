#include <Physics/PhysicsSystem.h>
#include <Core/ParticleRegistry.h>
#include <random>
#include <chrono>

static std::mt19937& getRng() {
    static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    return rng;
}

void PhysicsSystem::update(World& world, float deltaTime) {
    m_accumulator += deltaTime;

    m_accumulator = std::min(m_accumulator, 0.1f);

    while (m_accumulator >= FIXED_DT) {
        int worldWidth = world.getWidth();
        int worldHeight = world.getHeight();
        int totalCells = worldWidth * worldHeight;

        if (m_movedThisFrame.size() != totalCells) {
            m_movedThisFrame.resize(totalCells, 0);
        } else {
            std::fill(m_movedThisFrame.begin(), m_movedThisFrame.end(), 0);
        }

        for (int y = worldHeight - 1; y >= 0; --y) {
            const bool leftToRight = (y % 2 == 0);
            const int startX = leftToRight ? 0 : worldWidth - 1;
            const int endX = leftToRight ? worldWidth : -1;
            const int stepX = leftToRight ? 1 : -1;

            for (int x = startX; x != endX; x += stepX) {
                const int idx = y * worldWidth + x;
                if (m_movedThisFrame[idx]) continue;

                const ParticleId id = world.getParticleId(x, y);
                if (id == ParticleRegistry::Empty) {
                    world.resetAge(x, y);
                    continue;
                }

                world.incrementAge(x, y);

                const Vec2i oldPos(x, y);
                Vec2i newPos = oldPos;

                updateParticle(world, newPos);

                if (newPos.x != oldPos.x || newPos.y != oldPos.y) {
                    const int newIdx = newPos.y * worldWidth + newPos.x;
                    m_movedThisFrame[newIdx] = 1;
                    m_movedThisFrame[idx] = 1;
                }
            }
        }

        m_accumulator -= FIXED_DT;
    }

    // --- Temperature update ---
    m_tempAccumulator += deltaTime;
    float tempDt = 1.0f / m_tempUpdateRate;

    while (m_tempAccumulator >= tempDt) {
        updateTemperatures(world, tempDt);
        m_tempAccumulator -= tempDt;
    }
}

// ============== TEMPERATURE SYSTEM ==============

void PhysicsSystem::updateTemperatures(World& world, float dt) {
    const int width = world.getWidth();
    const int height = world.getHeight();
    const auto& registry = world.getRegistry();

    // OPTIMIZATION 1: Use a temporary buffer to avoid influencing neighbors in same step
    std::vector<uint8_t> newTemps(width * height, DEFAULT_TEMP);

    // OPTIMIZATION 2: Only process non-empty cells
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto& particle = world.getParticle(x, y);
            if (particle.id == ParticleRegistry::Empty) {
                newTemps[y * width + x] = DEFAULT_TEMP;
                continue;
            }

            const auto& def = registry.get(particle.id);
            float currentTemp = static_cast<float>(particle.temp);

            // Calculate average temperature of neighbors (4-directional)
            float avgTemp = getAverageNeighborTemp(world, x, y);

            // Thermal diffusion
            float delta = def.thermalConductivity * (avgTemp - currentTemp);
            float newTemp = currentTemp + delta * dt * 60.0f; // Normalize to 60fps

            // Self-heating sources (fire)
            if (def.state == PhysicalState::Fire) {
                newTemp += (def.flameTemp - currentTemp) * dt * 10.0f;
            }

            // Radiative cooling (heat loss to environment)
            if (currentTemp > DEFAULT_TEMP) {
                float cooling = (currentTemp - DEFAULT_TEMP) * def.emissivity * dt * 60.0f;
                newTemp -= cooling;
            }

            // Apply phase changes and ignition
            applyPhaseChange(world, x, y, newTemp, def);
            applyIgnition(world, x, y, newTemp);

            // Clamp temperature to valid range
            newTemps[y * width + x] = static_cast<uint8_t>(
                newTemp < 0.0f ? 0.0f : (newTemp > 255.0f ? 255.0f : newTemp)
            );
        }
    }

    // Apply new temperatures and mark cells as dirty
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t oldTemp = world.getParticle(x, y).temp;
            uint8_t newTemp = newTemps[y * width + x];

            if (oldTemp != newTemp) {
                world.getParticle(x, y).temp = newTemp;
                world.markDirty(x, y);
            }
        }
    }
}

float PhysicsSystem::getAverageNeighborTemp(const World& world, int x, int y) const {
    const int width = world.getWidth();
    const int height = world.getHeight();

    float sum = 0.0f;
    int count = 0;

    // Check 4 neighbors (up, down, left, right)
    const int dx[] = {0, 0, -1, 1};
    const int dy[] = {-1, 1, 0, 0};

    for (int i = 0; i < 4; ++i) {
        int nx = x + dx[i];
        int ny = y + dy[i];

        if (world.isInside(nx, ny)) {
            sum += static_cast<float>(world.getParticle(nx, ny).temp);
            count++;
        }
    }

    // If no neighbors, use room temperature
    if (count == 0) return DEFAULT_TEMP;

    return sum / count;
}

void PhysicsSystem::applyIgnition(World& world, int x, int y, float temp) {
    const auto& registry = world.getRegistry();
    const auto& def = registry.get(world.getParticleId(x, y));

    // Check if this material can ignite
    if (def.ignitionTemp <= 0.0f) return;
    if (temp < def.ignitionTemp) return;

    // Random chance based on temperature excess
    auto& rng = getRng();
    float excess = (temp - def.ignitionTemp) / 50.0f; // 0-1 range
    float chance = 0.001f * excess; // 0.1% chance per frame at ignition temp

    if (std::uniform_real_distribution<float>(0.0f, 1.0f)(rng) < chance) {
        ParticleId fireId = registry.findId("Fire");
        if (fireId != ParticleRegistry::Empty) {
            world.setParticle(x, y, fireId, static_cast<uint8_t>(temp));
        }
    }
}

void PhysicsSystem::applyPhaseChange(World& world, int x, int y, float temp, const ParticleDefinition& def) {
    const auto& registry = world.getRegistry();

    // Water -> Steam (evaporation)
    /*if (def.name == "Water" && temp > 210.0f) {
        auto& rng = getRng();
        if (std::uniform_real_distribution<float>(0.0f, 1.0f)(rng) < 0.05f) {
            ParticleId steamId = registry.findId("Steam"); // Use Smoke as steam for now
            if (steamId != ParticleRegistry::Empty) {
                world.setParticle(x, y, steamId, static_cast<uint8_t>(temp));
            }
        }
        return;
    }

    // Steam -> Water (condensation)
    if (def.name == "Steam" && temp < 100.0f) {
        auto& rng = getRng();
        if (std::uniform_real_distribution<float>(0.0f, 1.0f)(rng) < 0.02f) {
            ParticleId waterId = registry.findId("Water");
            if (waterId != ParticleRegistry::Empty) {
                world.setParticle(x, y, waterId, static_cast<uint8_t>(temp));
            }
        }
        return;
    }*/
}

void PhysicsSystem::updateParticle(World& world, const Vec2i& pos) {
    ParticleInstance& p = world.getParticle(pos.x, pos.y);
    const auto& registry = world.getRegistry();
    const auto& def = registry.get(p.id);

    switch (def.state) {
        case PhysicalState::Powder:
            updatePowder(world, pos);
            break;
        case PhysicalState::Liquid:
            updateLiquid(world, pos);
            break;
        case PhysicalState::Gas:
            updateGas(world, pos);
            break;
        case PhysicalState::Fire:
            updateFire(world, pos);
            break;
        default:
            break;
    }
}

void PhysicsSystem::updatePowder(World& world, const Vec2i& pos) {
    auto& rng = getRng();
    std::uniform_int_distribution<int> dist(-1, 1);
    int dx = dist(rng);

    Vec2i down(pos.x, pos.y + 1);
    if (tryMove(world, pos, down)) return;

    if (pos.y + 2 < world.getHeight()) {
        Vec2i fastDown(pos.x, pos.y + 2);
        if (world.getParticleId(fastDown.x, fastDown.y) == ParticleRegistry::Empty) {
            if (tryMove(world, pos, fastDown)) return;
        }
    }

    Vec2i downRight(pos.x + dx, pos.y + 1);
    if (tryMove(world, pos, downRight)) return;

    Vec2i downLeft(pos.x - dx, pos.y + 1);
    if (tryMove(world, pos, downLeft)) return;
}

void PhysicsSystem::updateLiquid(World& world, const Vec2i& pos) {
    auto& rng = getRng();
    int dx = std::uniform_int_distribution<>(-1, 1)(rng);

    Vec2i down(pos.x, pos.y + 1);
    if (tryMove(world, pos, down)) return;

    if (pos.y + 2 < world.getHeight()) {
        Vec2i fastDown(pos.x, pos.y + 2);
        if (world.getParticleId(fastDown.x, fastDown.y) == ParticleRegistry::Empty) {
            if (tryMove(world, pos, fastDown)) return;
        }
    }

    Vec2i right(pos.x + dx, pos.y);
    if (tryMove(world, pos, right)) return;

    if (dx != 0) {
        Vec2i left(pos.x - dx, pos.y);
        if (tryMove(world, pos, left)) return;
    }

    Vec2i downRight(pos.x + dx, pos.y + 1);
    if (tryMove(world, pos, downRight)) return;

    if (dx != 0) {
        Vec2i downLeft(pos.x - dx, pos.y + 1);
        if (tryMove(world, pos, downLeft)) return;
    }
}

void PhysicsSystem::updateGas(World& world, const Vec2i& pos) {
    auto& rng = getRng();
    std::uniform_int_distribution<int> dist(-1, 1);
    int dx = dist(rng);

    std::uniform_int_distribution<int> dissipateChance(0, 99);
    if (dissipateChance(rng) < 5) {
        world.setParticle(pos.x, pos.y, ParticleRegistry::Empty);
        return;
    }

    Vec2i up(pos.x, pos.y - 1);
    if (tryMove(world, pos, up)) return;

    Vec2i right(pos.x + dx, pos.y);
    if (tryMove(world, pos, right)) return;

    Vec2i left(pos.x - dx, pos.y);
    if (tryMove(world, pos, left)) return;

    Vec2i upRight(pos.x + dx, pos.y - 1);
    if (tryMove(world, pos, upRight)) return;

    Vec2i upLeft(pos.x - dx, pos.y - 1);
    if (tryMove(world, pos, upLeft)) return;
}

void PhysicsSystem::updateFire(World& world, const Vec2i& pos) {
    auto& rng = getRng();
    int age = world.getAge(pos.x, pos.y);

    // Fire turns to smoke based on temperature and age
    float temp = static_cast<float>(world.getParticle(pos.x, pos.y).temp);
    float smokeChance = 5.0f + (255.0f - temp) * 0.1f; // Cooler fire = more smoke

    if (age > 20 && std::uniform_int_distribution<>(0, 99)(rng) < static_cast<int>(smokeChance)) {
        ParticleId smokeId = world.getRegistry().findId("Smoke");
        world.setParticle(pos.x, pos.y, smokeId != ParticleRegistry::Empty ? smokeId : ParticleRegistry::Empty);
        return;
    }

    static const Vec2i directions[] = {
        {0, -1}, {-1, 0}, {1, 0}, {-1, -1}, {1, -1}, {0, 1}
    };

    int indices[] = {0, 1, 2, 3, 4, 5};
    for (int i = 5; i > 0; --i) {
        int j = std::uniform_int_distribution<>(0, i)(rng);
        std::swap(indices[i], indices[j]);
    }

    for (int i = 0; i < 6; ++i) {
        const auto& dir = directions[indices[i]];
        Vec2i newPos(pos.x + dir.x, pos.y + dir.y);
        if (tryMove(world, pos, newPos)) return;
    }

    // Ignite oil and other flammable materials nearby
    ParticleId oilId = world.getRegistry().findId("Oil");
    if (oilId != ParticleRegistry::Empty && std::uniform_int_distribution<>(0, 99)(rng) < 15) {
        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                if (dx == 0 && dy == 0) continue;
                Vec2i neighbor(pos.x + dx, pos.y + dy);
                if (world.isInside(neighbor.x, neighbor.y)) {
                    ParticleId neighborId = world.getParticleId(neighbor.x, neighbor.y);
                    if (neighborId != ParticleRegistry::Empty) {
                        const auto& def = world.getRegistry().get(neighborId);
                        // Ignite if material has ignition temperature
                        if (def.ignitionTemp > 0.0f) {
                            ParticleId fireId = world.getRegistry().findId("Fire");
                            world.setParticle(neighbor.x, neighbor.y, fireId);
                        }
                    }
                }
            }
        }
    }
}

bool PhysicsSystem::tryMove(World& world, const Vec2i& from, const Vec2i& to) {
    if (!world.isInside(to.x, to.y)) return false;

    const int toIdx = to.y * world.getWidth() + to.x;
    if (m_movedThisFrame[toIdx]) return false;

    const auto& registry = world.getRegistry();
    const auto& fromP = world.getParticle(from.x, from.y);
    const auto& toP = world.getParticle(to.x, to.y);

    if (toP.id == ParticleRegistry::Empty) {
        return performSwap(world, from, to);
    }

    const auto& fromDef = registry.get(fromP.id);
    const auto& toDef = registry.get(toP.id);

    if (toDef.state == PhysicalState::Solid) return false;

    if (fromDef.density > toDef.density) {
        return performSwap(world, from, to);
    }

    return false;
}

bool PhysicsSystem::performSwap(World& world, const Vec2i& from, const Vec2i& to) {
    ParticleInstance& fromP = world.getParticle(from.x, from.y);
    ParticleInstance& toP = world.getParticle(to.x, to.y);

    std::swap(fromP, toP);

    world.markDirty(from.x, from.y);
    world.markDirty(to.x, to.y);

    int fromIdx = from.y * world.getWidth() + from.x;
    int toIdx = to.y * world.getWidth() + to.x;

    m_movedThisFrame[toIdx] = true;
    m_movedThisFrame[fromIdx] = true;

    return true;
}
