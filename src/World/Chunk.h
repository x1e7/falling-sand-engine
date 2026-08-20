#pragma once
#include "Core/ParticleTypes.h"
#include "Core/Math/Vector2.h"

constexpr int CHUNK_SIZE = 16

struct Chunk {
    ParticleInstance cells[CHUNK_SIZE * CHUNK_SIZE];

    bool isActive = true;
};
