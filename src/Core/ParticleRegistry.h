#pragma once

#include "Core/ParticleTypes.h"
#include <unordered_map>
#include <vector>

class ParticleRegistry {
public:
    ParticleId registerParticle(const ParticleDefinition& def) {
        ParticleId id = m_definitions.size();
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
    sand.color = 0xFFE8C87A;
    sand.canIgnite = false;
    registry.registerParticle(sand);

    ParticleDefinition water;
    water.name = "Water";
    water.state = PhysicalState::Liquid;
    water.density = 1000.0f;
    water.color = 0xFF40C8FF;
    water.canIgnite = false;
    registry.registerParticle(water);

    ParticleDefinition smoke;
    smoke.name = "Smoke";
    smoke.state = PhysicalState::Gas;
    smoke.density = 0.6f;
    smoke.color = 0xCCCCCCCC;
    smoke.canIgnite = false;
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
    wall.canIgnite = false;
    registry.registerParticle(wall);

    ParticleDefinition oil;
    oil.name = "Oil";
    oil.state = PhysicalState::Liquid;
    oil.density = 800.0f;
    oil.color = 0xFF8B4513;
    oil.canIgnite = true;
    registry.registerParticle(oil);
}
