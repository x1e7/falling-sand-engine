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

using ParticleId = uint32_t;

struct ParticleDefinition {
    std::string name;
    PhysicalState state = PhysicalState::Empty;
    float density = 0.0f;
    float viscosity = 0.0f;
    float restitution = 0.0f;
    float friction = 0.0f;
    float thermalConductivity = 0.0f;
    float heatCapacity = 0.0f;
    float meltingPoint = 0.0f;
    float boilingPoint = 0.0f;
    uint32_t color = 0xFFFFFFFF;
    
    ParticleDefinition() = default;
    
    ParticleDefinition(const std::string& n, PhysicalState s, float d)
        : name(n), state(s), density(d) {}
};

}