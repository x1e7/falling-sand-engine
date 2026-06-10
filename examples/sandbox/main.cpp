#include <Sand2D/Sand2D.h>
#include "Renderer.h"
#include <cstdlib>
#include <ctime>

int main() {
    Sand2D::ParticleRegistry registry;
    Sand2D::registerSand2DParticles(registry);
    
    Sand2D::World world(200, 150, registry);
    Sand2D::PhysicsSystem physics;
    
    Renderer renderer(1280, 720, "Sandbox - Physics Demo", registry);
    
    Sand2D::ParticleId sandId = registry.findId("Sand");
    Sand2D::ParticleId waterId = registry.findId("Water");
    Sand2D::ParticleId wallId = registry.findId("Wall");
    Sand2D::ParticleId emptyId = Sand2D::ParticleRegistry::Empty;
    
    for (int x = 95; x < 105; ++x)
        for (int y = 0; y < 20; ++y)
            world.setParticle(x, y, sandId);
    
    for (int y = 0; y < 30; ++y)
        world.setParticle(30, y, waterId);
    
    for (int y = 100; y < 150; ++y)
        world.setParticle(170, y, wallId);
    
    Sand2D::ParticleId currentBrush = sandId;
    int brushRadius = 1;
    
    while (renderer.isOpen()) {
        renderer.handleEvents();
        
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_1) == GLFW_PRESS)
            currentBrush = sandId;
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_2) == GLFW_PRESS)
            currentBrush = waterId;
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_3) == GLFW_PRESS)
            currentBrush = registry.findId("Fire");
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_4) == GLFW_PRESS)
            currentBrush = wallId;
        
        int x, y;
        renderer.getMouseWorldPosition(world, x, y);

        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_KP_ADD) == GLFW_PRESS || 
            glfwGetKey(renderer.getWindow(), GLFW_KEY_EQUAL) == GLFW_PRESS) {
            brushRadius = std::min(brushRadius + 1, 10); // max 10
            glfwWaitEventsTimeout(0.05);
        }
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS ||
            glfwGetKey(renderer.getWindow(), GLFW_KEY_MINUS) == GLFW_PRESS) {
            brushRadius = std::max(brushRadius - 1, 1);   // min 1
            glfwWaitEventsTimeout(0.05);
        }
        
        if (glfwGetMouseButton(renderer.getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            for (int dy = -brushRadius; dy <= brushRadius; dy++) {
                for (int dx = -brushRadius; dx <= brushRadius; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;
                    
                    if (dx*dx + dy*dy > brushRadius*brushRadius) continue;   // CIRCLE 
                    
                    if (world.isInside(nx, ny) && world.getParticleId(nx, ny) == emptyId) {
                        world.setParticle(nx, ny, currentBrush);
                        renderer.markDirty(nx, ny);
                    }
                }
            }
        }

        if (glfwGetMouseButton(renderer.getWindow(), GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            for (int dy = -brushRadius; dy <= brushRadius; dy++) {
                for (int dx = -brushRadius; dx <= brushRadius; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;
                    
                    if (dx*dx + dy*dy > brushRadius*brushRadius) continue; // CIRCLE 
                    
                    if (world.isInside(nx, ny) && world.getParticleId(nx, ny) != emptyId) {
                        world.setParticle(nx, ny, emptyId);
                        renderer.markDirty(nx, ny);
                    }
                }
            }
        }
        
        physics.update(world);
        renderer.render(world);
    }
    
    return 0;
}