#pragma once

#include <Sand2D/ParticleTypes.h>
#include <unordered_map>
#include <vector>

namespace Sand2D {

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
    sand.viscosity = 0.0f;
    sand.restitution = 0.3f;
    sand.friction = 0.6f;
    sand.color = 0xC4B280FF;
    registry.registerParticle(sand);
    
    ParticleDefinition water;
    water.name = "Water";
    water.state = PhysicalState::Liquid;
    water.density = 1000.0f;
    water.viscosity = 0.01f;
    water.restitution = 0.1f;
    water.friction = 0.3f;
    water.color = 0x40A4DFFF;
    registry.registerParticle(water);

    ParticleDefinition smoke;
    smoke.name = "Smoke";
    smoke.state = PhysicalState::Gas;
    smoke.density = 0.6f;
    smoke.color = 0x88888888;
    registry.registerParticle(smoke);

    ParticleDefinition fire;
    fire.name = "Fire";
    fire.state = PhysicalState::Fire;
    fire.density = 0.2f;           // Light - rises quickly
    fire.viscosity = 0.0f;
    fire.restitution = 0.0f;
    fire.friction = 0.0f;
    fire.color = 0xFF6600FF;        // Bright orange
    registry.registerParticle(fire);
    
    ParticleDefinition wall;
    wall.name = "Wall";
    wall.state = PhysicalState::Solid;
    wall.density = 999999.0f;
    wall.color = 0x646464FF;
    registry.registerParticle(wall);
}

}