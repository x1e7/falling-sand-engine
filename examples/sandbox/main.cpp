#include <Sand2D/Sand2D.h>
#include "Renderer.h"
#include <cstdlib>
#include <ctime>

int main() {
    Sand2D::ParticleRegistry registry;
    Sand2D::registerSand2DParticles(registry);

    // World size matches window size for perfect pixel mapping
    const int WORLD_WIDTH = 600;
    const int WORLD_HEIGHT = 450;

    Sand2D::World world(WORLD_WIDTH, WORLD_HEIGHT, registry);
    Sand2D::PhysicsSystem physics;
    Renderer renderer(WORLD_WIDTH, WORLD_HEIGHT, "Sandbox - Physics Demo", registry);

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

    while (renderer.isOpen()) {
        renderer.handleEvents();

        // Brush selection
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_1) == GLFW_PRESS)
            currentBrush = sandId;
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_2) == GLFW_PRESS)
            currentBrush = waterId;
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_3) == GLFW_PRESS)
            currentBrush = registry.findId("Fire");
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_4) == GLFW_PRESS)
            currentBrush = wallId;

        // Save/Load (Ctrl+S / Ctrl+L)
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_S) == GLFW_PRESS &&
            glfwGetKey(renderer.getWindow(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
            Sand2D::WorldSerializer::saveWorld(world, "world.bin");
        }
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_L) == GLFW_PRESS &&
            glfwGetKey(renderer.getWindow(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
            Sand2D::WorldSerializer::loadWorld(world, "world.bin");
        }

        // Mouse position in world coordinates
        int x, y;
        renderer.getMouseWorldPosition(world, x, y);

        // Brush size
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_KP_ADD) == GLFW_PRESS ||
            glfwGetKey(renderer.getWindow(), GLFW_KEY_EQUAL) == GLFW_PRESS) {
            brushRadius = std::min(brushRadius + 1, 10);
            glfwWaitEventsTimeout(0.05);
        }
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS ||
            glfwGetKey(renderer.getWindow(), GLFW_KEY_MINUS) == GLFW_PRESS) {
            brushRadius = std::max(brushRadius - 1, 1);
            glfwWaitEventsTimeout(0.05);
        }

        // Left click: place particles
        if (glfwGetMouseButton(renderer.getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
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
        if (glfwGetMouseButton(renderer.getWindow(), GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
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
