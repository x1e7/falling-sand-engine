#pragma once

#include <Core/ParticleTypes.h>
#include <Core/ParticleRegistry.h>

#include <Core/Math/Vector2.h>

#include <World/World.h>

#include <Physics/PhysicsSystem.h>

#include <Serialization/WorldSerializer.h>
#include <Serialization/Compression.h>

inline const char* getVersion() {
    return "1.0.0";
}
