#pragma once

#include "Core/Particle.h"

constexpr int CHUNK_SIZE  = 64;
constexpr int CHUNK_SHIFT = 6;              // log2(CHUNK_SIZE)
constexpr int CHUNK_MASK  = CHUNK_SIZE - 1;
constexpr int CHUNK_AREA  = CHUNK_SIZE * CHUNK_SIZE;

static_assert((1 << CHUNK_SHIFT) == CHUNK_SIZE, "CHUNK_SHIFT must match CHUNK_SIZE");

struct Chunk {
    ParticleInstance cells[CHUNK_AREA];
    int idleFrames = 0;

    Chunk() {
        for (int i = 0; i < CHUNK_AREA; ++i) {
            cells[i] = ParticleInstance{};
        }
    }
};
