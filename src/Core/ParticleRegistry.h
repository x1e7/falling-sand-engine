#pragma once

#include "Core/ParticleTypes.h"
#include <unordered_map>
#include <vector>

class ParticleRegistry {
public:
    ParticleId registerParticle(const ParticleDefinition& def) {
        ParticleId id = static_cast<ParticleId>(m_definitions.size());
        m_definitions.push_back(def);
        m_nameToId[def.name] = id;
        return id;
    }

    const ParticleDefinition& get(ParticleId id) const {
        static ParticleDefinition empty;
        if (id >= m_definitions.size()) return empty;
        return m_definitions[id];
    }

    ParticleId findId(const std::string& name) const {
        auto it = m_nameToId.find(name);
        return it != m_nameToId.end() ? it->second : Empty;
    }

    const std::vector<ParticleDefinition>& getAll() const {
        return m_definitions;
    }

    void setBackgroundColor(uint32_t color) {
        m_definitions[0].color = color;
    }

    static constexpr ParticleId Empty = 0;

private:
    std::vector<ParticleDefinition> m_definitions = {
        ParticleDefinition("Empty", PhysicalState::Empty, 0.0f)
    };
    std::unordered_map<std::string, ParticleId> m_nameToId;
};

inline void registerSand2DParticles(ParticleRegistry& registry) {
    ParticleDefinition sand;
    sand.name = "Sand";
    sand.state = PhysicalState::Powder;
    sand.density = 1600.0f;
    sand.color = 0xFFBF9960;
    registry.registerParticle(sand);

    ParticleDefinition water;
    water.name = "Water";
    water.state = PhysicalState::Liquid;
    water.density = 1000.0f;
    water.color = 0xD24C7ABE;
    registry.registerParticle(water);

    ParticleDefinition smoke;
    smoke.name = "Smoke";
    smoke.state = PhysicalState::Gas;
    smoke.density = 0.6f;
    smoke.color = 0xCCCCCCCC;
    registry.registerParticle(smoke);

    ParticleDefinition fire;
    fire.name = "Fire";
    fire.state = PhysicalState::Fire;
    fire.density = 0.2f;
    fire.color = 0xFFFF8C00;
    fire.canIgnite = false;
    registry.registerParticle(fire);

    ParticleDefinition wall;
    wall.name = "Wall";
    wall.state = PhysicalState::Solid;
    wall.density = 999999.0f;
    wall.color = 0xFF8899AA;
    wall.isCorrodible = false;
    registry.registerParticle(wall);

    ParticleDefinition oil;
    oil.name = "Oil";
    oil.state = PhysicalState::Liquid;
    oil.density = 800.0f;
    oil.color = 0xFF8B4513;
    oil.canIgnite = true;
    registry.registerParticle(oil);

    // Stone
    ParticleDefinition stone;
    stone.name = "Stone";
    stone.state = PhysicalState::Solid;
    stone.density = 3000.0f;
    stone.color = 0xFF808080;
    stone.canMelt = true;
    stone.meltInto = "Lava";
    registry.registerParticle(stone);

    // Lava
    ParticleDefinition lava;
    lava.name = "Lava";
    lava.state = PhysicalState::Liquid;
    lava.density = 2500.0f;
    lava.color = 0xFFFF4500;
    lava.isHot = true;
    registry.registerParticle(lava);

    // Wood
    ParticleDefinition wood;
    wood.name = "Wood";
    wood.state = PhysicalState::Solid;
    wood.density = 700.0f;
    wood.color = 0xFF8B6B4A;
    wood.canIgnite = true;
    wood.burnInto = "Fire";
    registry.registerParticle(wood);

    // Acid
    ParticleDefinition acid;
    acid.name = "Acid";
    acid.state = PhysicalState::Liquid;
    acid.density = 1200.0f;
    acid.color = 0xFF00FF00;
    acid.isCorrosive = true;
    acid.isCorrodible = false;
    registry.registerParticle(acid);

    // Dust
    ParticleDefinition dust;
    dust.name = "Dust";
    dust.state = PhysicalState::Powder;
    dust.density = 500.0f;
    dust.color = 0xFFC4A882;
    dust.canIgnite = true;
    registry.registerParticle(dust);

    // Plant
    ParticleDefinition plant;
    plant.name = "Plant";
    plant.state = PhysicalState::Solid;
    plant.density = 200.0f;
    plant.color = 0xFF32CD32;
    plant.canIgnite = true;
    plant.burnInto = "Fire";
    registry.registerParticle(plant);

    // Seed
    ParticleDefinition seed;
    seed.name = "Seed";
    seed.state = PhysicalState::Powder;
    seed.density = 600.0f;
    seed.color = 0xFF8B7D3C;
    seed.canIgnite = true;
    registry.registerParticle(seed);

    // Gas
    ParticleDefinition gas;
    gas.name = "Gas";
    gas.state = PhysicalState::Gas;
    gas.density = 0.1f;
    gas.color = 0xB4C0C0C0;
    gas.canIgnite = true;
    registry.registerParticle(gas);
}
