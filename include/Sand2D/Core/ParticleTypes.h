#pragma once

#include <string>
#include <cstdint>

namespace Sand2D {

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

    ParticleDefinition() = default;

    ParticleDefinition(const std::string& n, PhysicalState s, float d)
        : name(n), state(s), density(d) {}
};

}
