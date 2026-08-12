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

    // Thermal properties
    float thermalConductivity = 0.2f;   // 0 = insulator, 1 = superconductor
    float heatCapacity = 0.5f;          // 0 = instant heat, 1 = slow heat
    float ignitionTemp = 0.0f;          // 0 = non-flammable, >0 = ignition point
    float flameTemp = 200.0f;           // Temperature fire maintains
    float emissivity = 0.05f;           // 0-1, how fast it cools by radiation

    ParticleDefinition() = default;

    ParticleDefinition(const std::string& n, PhysicalState s, float d)
        : name(n), state(s), density(d) {}
};
