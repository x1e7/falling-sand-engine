#pragma once

#include "Core/ParticleInstance.h"
#include "Core/ParticleTypes.h"
#include "Core/Math/Vector2.h"

constexpr int CHUNK_SIZE = 16;

struct Chunk {
    ParticleInstance cells[CHUNK_SIZE * CHUNK_SIZE];

    int idleFrames = 0;

    Chunk() {
        for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; ++i) {
            cells[i].id = ParticleRegistry::Empty;
            cells[i].age = 0;
        }
    }
};
