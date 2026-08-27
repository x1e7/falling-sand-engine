#pragma once

#include "ParticleTypes.h"
#include <cstdint>

struct ParticleInstance {
    ParticleId id;
    uint8_t age;
    uint8_t brightness;
};
