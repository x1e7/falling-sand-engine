#include <Sand2D/Sand2D.h>
#include "Render/Renderer.h"
#include "Render/UIRenderer.h"

int main(int argc, char* argv[]) {
    Sand2D::ParticleRegistry registry;
    registry.setBackgroundColor(0xFF2A2A2A);
    Sand2D::registerSand2DParticles(registry);

    const int WORLD_WIDTH = 800;
    const int WORLD_HEIGHT = 600;
    const int WINDOW_WIDTH = 1280;
    const int WINDOW_HEIGHT = 720;

    Sand2D::World world(WORLD_WIDTH, WORLD_HEIGHT, registry);
    Sand2D::PhysicsSystem physics;

    Renderer renderer(WORLD_WIDTH, WORLD_HEIGHT, WINDOW_WIDTH, WINDOW_HEIGHT,
                     "Sandbox - Physics Demo", registry);
    UIRenderer ui(renderer.getRenderer(), WINDOW_WIDTH, WINDOW_HEIGHT);

    Sand2D::ParticleId emptyId = Sand2D::ParticleRegistry::Empty;
    Sand2D::ParticleId sandId = registry.findId("Sand");
    Sand2D::ParticleId waterId = registry.findId("Water");
    Sand2D::ParticleId fireId = registry.findId("Fire");
    Sand2D::ParticleId wallId = registry.findId("Wall");
    Sand2D::ParticleId smokeId = registry.findId("Smoke");
    Sand2D::ParticleId oilId = registry.findId("Oil");

    Sand2D::WorldSerializer::loadWorld(world, "world.bin");

    Sand2D::ParticleId currentBrush = sandId;
    int brushRadius = 1;
    const int MAX_BRUSH_RADIUS = 128;
    const int MIN_BRUSH_RADIUS = 1;

    Uint32 lastBrushChange = 0;
    Uint32 lastSaveLoad = 0;
    const Uint32 BRUSH_COOLDOWN = 50; // ms
    const Uint32 SAVE_COOLDOWN = 2500; // ms

    const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
    bool ctrlPressed = false;

    int fps = 0;
    int frameCount = 0;
    Uint32 lastTime = SDL_GetTicks();

    renderer.setWheelCallback([&](int direction) {
        if (direction > 0) {
            brushRadius = std::min(brushRadius + 1, MAX_BRUSH_RADIUS);
        } else if (direction < 0) {
            brushRadius = std::max(brushRadius - 1, MIN_BRUSH_RADIUS);
        }
    });

    while (renderer.isOpen()) {
        renderer.handleEvents();

        keyboardState = SDL_GetKeyboardState(nullptr);
        ctrlPressed = keyboardState[SDL_SCANCODE_LCTRL] || keyboardState[SDL_SCANCODE_RCTRL];

        if (keyboardState[SDL_SCANCODE_1]) currentBrush = sandId;
        if (keyboardState[SDL_SCANCODE_2]) currentBrush = waterId;
        if (keyboardState[SDL_SCANCODE_3]) currentBrush = fireId;
        if (keyboardState[SDL_SCANCODE_4]) currentBrush = wallId;
        if (keyboardState[SDL_SCANCODE_5]) currentBrush = oilId;

        int mouseX, mouseY;
        renderer.getMouseWorldPosition(world, mouseX, mouseY);

        Uint32 currentTime = SDL_GetTicks();

        if (ctrlPressed && currentTime - lastSaveLoad > SAVE_COOLDOWN) {
            if (keyboardState[SDL_SCANCODE_S]) {
                Sand2D::WorldSerializer::saveWorld(world, "world.bin");
                lastSaveLoad = currentTime;
            }
            if (keyboardState[SDL_SCANCODE_L]) {
                Sand2D::WorldSerializer::loadWorld(world, "world.bin");
                lastSaveLoad = currentTime;
            }
        }

        Uint32 mouseState = SDL_GetMouseState(nullptr, nullptr);

        if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT) && mouseX >= 0 && mouseY >= 0) {
            for (int dy = -brushRadius; dy <= brushRadius; dy++) {
                for (int dx = -brushRadius; dx <= brushRadius; dx++) {
                    int nx = mouseX + dx;
                    int ny = mouseY + dy;
                    if (dx * dx + dy * dy > brushRadius * brushRadius) continue;
                    if (world.isInside(nx, ny)
                        && (world.getParticleId(nx, ny) == emptyId
                        || world.getParticleId(nx, ny) == smokeId)) {
                        world.setParticle(nx, ny, currentBrush);
                    }
                }
            }
        }

        if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT) && mouseX >= 0 && mouseY >= 0) {
            for (int dy = -brushRadius; dy <= brushRadius; dy++) {
                for (int dx = -brushRadius; dx <= brushRadius; dx++) {
                    int nx = mouseX + dx;
                    int ny = mouseY + dy;
                    if (dx * dx + dy * dy > brushRadius * brushRadius) continue;
                    if (world.isInside(nx, ny)) {
                        world.setParticle(nx, ny, emptyId);
                    }
                }
            }
        }

        physics.update(world);

        // === FPS ===
        frameCount++;
        if (SDL_GetTicks() - lastTime >= 1000) {
            fps = frameCount;
            frameCount = 0;
            lastTime = SDL_GetTicks();
        }

        // === UI ===
        ui.setFPS(fps);

        std::string brushName = "Unknown";
        if (currentBrush == sandId) brushName = "Sand";
        else if (currentBrush == waterId) brushName = "Water";
        else if (currentBrush == registry.findId("Fire")) brushName = "Fire";
        else if (currentBrush == wallId) brushName = "Wall";
        else if (currentBrush == oilId) brushName = "Oil";
        ui.setBrushInfo(brushName, brushRadius);

        renderer.render(world, &ui);
    }

    return 0;
}
