#pragma once

#include <string>
#include <cstdint>

constexpr uint8_t DEFAULT_TEMP = 0;

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

    bool canIgnite = 0.0f;

    ParticleDefinition() = default;

    ParticleDefinition(const std::string& n, PhysicalState s, float d)
        : name(n), state(s), density(d) {}
};
