#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

using ParticleId = uint8_t;

struct ParticleInstance {
    ParticleId id = 0;
    uint8_t age = 0;
    uint8_t brightness = 0;
};

enum class PhysicalState : uint8_t {
    Empty,
    Solid,
    Powder,
    Fire,
    Liquid,
    Gas
};

namespace ParticleFlags {
    inline constexpr uint16_t None         = 0;
    inline constexpr uint16_t CanIgnite    = 1u << 0;
    inline constexpr uint16_t CanMelt      = 1u << 1;
    inline constexpr uint16_t IsHot        = 1u << 2;
    inline constexpr uint16_t IsCorrosive  = 1u << 3;
    inline constexpr uint16_t IsCorrodible = 1u << 4;
    inline constexpr uint16_t IsAlive      = 1u << 5;
}

struct ParticleDefinition {
    std::string name;
    PhysicalState state = PhysicalState::Empty;
    float density = 0.0f;
    uint32_t color = 0xFFFFFFFF;
    bool canIgnite = false;
    bool canMelt = false;
    ParticleId meltInto = 0;
    bool isHot = false;
    bool isCorrosive = false;
    bool isCorrodible = true;
    bool isAlive = false;
    float growthRate = 0.0f;

    ParticleDefinition() = default;
    ParticleDefinition(const std::string& n, PhysicalState s, float d)
        : name(n), state(s), density(d) {}
};

struct ParticlePhysics {
    PhysicalState state = PhysicalState::Empty;
    uint16_t flags = ParticleFlags::IsCorrodible;
    ParticleId meltInto = 0;
    float density = 0.0f;
};

class ParticleRegistry {
public:
    static constexpr ParticleId Empty = 0;
    static constexpr size_t MaxParticles = 256;

    ParticleRegistry() { rebuildPhysics(); }

    ParticleId registerParticle(const ParticleDefinition& def) {
        assert(m_definitions.size() < MaxParticles && "particle limit reached");
        ParticleId id = static_cast<ParticleId>(m_definitions.size());
        m_definitions.push_back(def);
        m_nameToId[def.name] = id;
        rebuildPhysics();
        return id;
    }

    const ParticleDefinition& get(ParticleId id) const {
        static const ParticleDefinition empty{};
        if (id >= m_definitions.size()) return empty;
        return m_definitions[id];
    }

    const ParticlePhysics& physics(ParticleId id) const { return m_physics[id]; }

    bool canDisplace(ParticleId from, ParticleId to) const {
        return (m_displace[from][to >> 5] & (1u << (to & 31))) != 0;
    }

    ParticleId findId(const std::string& name) const {
        auto it = m_nameToId.find(name);
        return it != m_nameToId.end() ? it->second : Empty;
    }

    ParticleId requireId(const std::string& name) const {
        ParticleId id = findId(name);
        assert(id != Empty && "particle referenced before registration");
        return id;
    }

    const std::vector<ParticleDefinition>& getAll() const { return m_definitions; }

    void setBackgroundColor(uint32_t color) { m_definitions[0].color = color; }

private:
    std::vector<ParticleDefinition> m_definitions = {
        ParticleDefinition("Empty", PhysicalState::Empty, 0.0f)
    };
    std::unordered_map<std::string, ParticleId> m_nameToId;
    std::array<ParticlePhysics, MaxParticles> m_physics{};
    std::array<std::array<uint32_t, MaxParticles / 32>, MaxParticles> m_displace{};

    void rebuildPhysics() {
        for (size_t i = 0; i < MaxParticles; ++i) {
            ParticlePhysics ph{};
            if (i < m_definitions.size()) {
                const ParticleDefinition& d = m_definitions[i];
                ph.state = d.state;
                ph.density = d.density;
                ph.meltInto = d.meltInto;
                ph.flags = ParticleFlags::None;
                if (d.canIgnite)    ph.flags |= ParticleFlags::CanIgnite;
                if (d.canMelt)      ph.flags |= ParticleFlags::CanMelt;
                if (d.isHot)        ph.flags |= ParticleFlags::IsHot;
                if (d.isCorrosive)  ph.flags |= ParticleFlags::IsCorrosive;
                if (d.isCorrodible) ph.flags |= ParticleFlags::IsCorrodible;
                if (d.isAlive)      ph.flags |= ParticleFlags::IsAlive;
            }
            m_physics[i] = ph;
        }

        for (size_t a = 0; a < MaxParticles; ++a) {
            m_displace[a].fill(0);
            const ParticlePhysics& from = m_physics[a];
            if (from.state == PhysicalState::Empty || from.state == PhysicalState::Solid) continue;
            for (size_t b = 0; b < MaxParticles; ++b) {
                if (!displaceRule(from, m_physics[b])) continue;
                m_displace[a][b >> 5] |= (1u << (b & 31));
            }
        }
    }

    static bool displaceRule(const ParticlePhysics& from, const ParticlePhysics& to) {
        if (to.state == PhysicalState::Empty) return true;
        if (to.state == PhysicalState::Solid) return false;
        if (from.state == PhysicalState::Gas) return true;
        if (to.state == PhysicalState::Gas) return true;
        if (from.state == PhysicalState::Liquid && to.state == PhysicalState::Powder) return false;
        if (from.state == PhysicalState::Powder && to.state == PhysicalState::Liquid) return true;
        return from.density > to.density;
    }
};

inline void registerSand2DParticles(ParticleRegistry& registry) {
    ParticleDefinition sand;
    sand.name = "Sand";
    sand.state = PhysicalState::Powder;
    sand.density = 1600.0f;
    sand.color = 0xFFBF9960;
    registry.registerParticle(sand);

    ParticleDefinition steam;
    steam.name = "Steam";
    steam.state = PhysicalState::Gas;
    steam.density = 0.6f;
    steam.color = 0xCCCCCCCC;
    registry.registerParticle(steam);

    ParticleDefinition water;
    water.name = "Water";
    water.state = PhysicalState::Liquid;
    water.density = 1000.0f;
    water.color = 0xD24C7ABE;
    water.canMelt = true;
    water.meltInto = registry.requireId("Steam");
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
    oil.canMelt = true;
    oil.meltInto = registry.requireId("Fire");
    registry.registerParticle(oil);

    ParticleDefinition lava;
    lava.name = "Lava";
    lava.state = PhysicalState::Liquid;
    lava.density = 2500.0f;
    lava.color = 0xFFFF4500;
    lava.isHot = true;
    registry.registerParticle(lava);

    ParticleDefinition stone;
    stone.name = "Stone";
    stone.state = PhysicalState::Solid;
    stone.density = 3000.0f;
    stone.color = 0xFF808080;
    stone.canMelt = true;
    stone.meltInto = registry.requireId("Lava");
    registry.registerParticle(stone);

    ParticleDefinition wood;
    wood.name = "Wood";
    wood.state = PhysicalState::Solid;
    wood.density = 700.0f;
    wood.color = 0xFF8B6B4A;
    wood.canIgnite = true;
    wood.canMelt = true;
    wood.meltInto = registry.requireId("Fire");
    registry.registerParticle(wood);

    ParticleDefinition acid;
    acid.name = "Acid";
    acid.state = PhysicalState::Liquid;
    acid.density = 1200.0f;
    acid.color = 0xFF00FF00;
    acid.isCorrosive = true;
    acid.isCorrodible = false;
    registry.registerParticle(acid);

    ParticleDefinition dust;
    dust.name = "Dust";
    dust.state = PhysicalState::Powder;
    dust.density = 500.0f;
    dust.color = 0xFFC4A882;
    dust.canIgnite = true;
    dust.canMelt = true;
    dust.meltInto = registry.requireId("Fire");
    registry.registerParticle(dust);

    ParticleDefinition plant;
    plant.name = "Plant";
    plant.state = PhysicalState::Solid;
    plant.density = 200.0f;
    plant.color = 0xFF32CD32;
    plant.canIgnite = true;
    plant.canMelt = true;
    plant.meltInto = registry.requireId("Fire");
    registry.registerParticle(plant);

    ParticleDefinition seed;
    seed.name = "Seed";
    seed.state = PhysicalState::Powder;
    seed.density = 600.0f;
    seed.color = 0xFF8B7D3C;
    seed.canIgnite = true;
    seed.canMelt = true;
    seed.meltInto = registry.requireId("Fire");
    registry.registerParticle(seed);

    ParticleDefinition gas;
    gas.name = "Gas";
    gas.state = PhysicalState::Gas;
    gas.density = 0.1f;
    gas.color = 0xB4C0C0C0;
    gas.canIgnite = true;
    gas.canMelt = true;
    gas.meltInto = registry.requireId("Fire");
    registry.registerParticle(gas);
}
