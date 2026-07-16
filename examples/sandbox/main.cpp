#include <Sand2D/Sand2D.h>
#include "Render/Renderer.h"

int main(int argc, char* argv[]) {
    Sand2D::ParticleRegistry registry;
    registry.setBackgroundColor(0xFF2A2A2A);
    Sand2D::registerSand2DParticles(registry);

    // World size matches window size for perfect pixel mapping
    const int WORLD_WIDTH = 600;
    const int WORLD_HEIGHT = 450;
    const int WINDOW_WIDTH = 1280;
    const int WINDOW_HEIGHT = 720;

    Sand2D::World world(WORLD_WIDTH, WORLD_HEIGHT, registry);
    Sand2D::PhysicsSystem physics;
    Renderer renderer(WORLD_WIDTH, WORLD_HEIGHT, WINDOW_WIDTH, WINDOW_HEIGHT, "Sandbox - Physics Demo", registry);

    Sand2D::ParticleId sandId = registry.findId("Sand");
    Sand2D::ParticleId waterId = registry.findId("Water");
    Sand2D::ParticleId wallId = registry.findId("Wall");
    Sand2D::ParticleId emptyId = Sand2D::ParticleRegistry::Empty;

    // Load save or create initial world
    if (!Sand2D::WorldSerializer::loadWorld(world, "world.bin")) {
        // Sand pile
        for (int x = 95; x < 105; ++x)
            for (int y = 0; y < 20; ++y)
                world.setParticle(x, y, sandId);

        // Water column
        for (int y = 0; y < 30; ++y)
            world.setParticle(30, y, waterId);

        // Wall
        for (int y = 100; y < 150; ++y)
            world.setParticle(170, y, wallId);
    }

    Sand2D::ParticleId currentBrush = sandId;
    int brushRadius = 1;

    const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
    bool ctrlPressed = false;

    while (renderer.isOpen()) {
        renderer.handleEvents();

        keyboardState = SDL_GetKeyboardState(nullptr);
        ctrlPressed = keyboardState[SDL_SCANCODE_LCTRL] || keyboardState[SDL_SCANCODE_RCTRL];

        // Brush selection
        if (keyboardState[SDL_SCANCODE_1]) currentBrush = sandId;
        if (keyboardState[SDL_SCANCODE_2]) currentBrush = waterId;
        if (keyboardState[SDL_SCANCODE_3]) currentBrush = registry.findId("Fire");
        if (keyboardState[SDL_SCANCODE_4]) currentBrush = wallId;

        // Save/Load (Ctrl+S / Ctrl+L)
        if (ctrlPressed && keyboardState[SDL_SCANCODE_S]) {
            Sand2D::WorldSerializer::saveWorld(world, "world.bin");
        }
        if (ctrlPressed && keyboardState[SDL_SCANCODE_L]) {
            Sand2D::WorldSerializer::loadWorld(world, "world.bin");
        }

        // Mouse position in world coordinates
        int x, y;
        renderer.getMouseWorldPosition(world, x, y);

        // Brush size
        if (keyboardState[SDL_SCANCODE_EQUALS] || keyboardState[SDL_SCANCODE_KP_PLUS]) {
            brushRadius = std::min(brushRadius + 1, 10);
            SDL_Delay(50);
        }
        if (keyboardState[SDL_SCANCODE_MINUS] || keyboardState[SDL_SCANCODE_KP_MINUS]) {
            brushRadius = std::max(brushRadius - 1, 1);
            SDL_Delay(50);
        }

        Uint32 mouseState = SDL_GetMouseState(nullptr, nullptr);

        // Left click: place particles
        if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT) && x >= 0 && y >= 0) {
            for (int dy = -brushRadius; dy <= brushRadius; dy++) {
                for (int dx = -brushRadius; dx <= brushRadius; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (dx * dx + dy * dy > brushRadius * brushRadius) continue;

                    if (world.isInside(nx, ny) && world.getParticleId(nx, ny) == emptyId) {
                        world.setParticle(nx, ny, currentBrush);
                    }
                }
            }
        }

        // Right click: erase particles
        if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT) && x >= 0 && y >= 0) {
            for (int dy = -brushRadius; dy <= brushRadius; dy++) {
                for (int dx = -brushRadius; dx <= brushRadius; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (dx * dx + dy * dy > brushRadius * brushRadius) continue;

                    if (world.isInside(nx, ny) && world.getParticleId(nx, ny) != emptyId) {
                        world.setParticle(nx, ny, emptyId);
                    }
                }
            }
        }

        physics.update(world);
        renderer.render(world);
    }

    return 0;
}
