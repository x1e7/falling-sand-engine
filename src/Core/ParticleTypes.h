#pragma once

#include <string>
#include <cstdint>

enum class PhysicalState {
    Empty,
    Solid,
    Powder,
    Fire,
    Liquid,
    Gas
};

using ParticleId = uint8_t;

struct ParticleDefinition {
    std::string name;
    PhysicalState state = PhysicalState::Empty;
    float density = 0.0f;
    uint32_t color = 0xFFFFFFFF;

    bool canIgnite = false;
    bool canMelt = false;
    std::string meltInto;
    bool isHot = false;
    bool isCorrosive = false;
    bool isCorrodible = true;
    std::string burnInto;
    bool isAlive = false;
    float growthRate = 0.0f;

    ParticleDefinition() = default;

    ParticleDefinition(const std::string& n, PhysicalState s, float d)
        : name(n), state(s), density(d) {}
};
