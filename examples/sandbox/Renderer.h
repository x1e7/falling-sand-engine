#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Sand2D/World.h>
#include <string>
#include <vector>
#include <unordered_set>

class Renderer {
public:
    Renderer(int width, int height, const std::string& title, Sand2D::ParticleRegistry& registry);
    ~Renderer();

    void render(Sand2D::World& world);
    void handleEvents();
    void markDirty(int x, int y) { m_dirtyCells.insert(y * 10000 + x); }
    
    bool isOpen() const { return !glfwWindowShouldClose(m_window); }
    GLFWwindow* getWindow() { return m_window; }
    void getMouseWorldPosition(Sand2D::World& world, int& x, int& y) const;
    
private:
    void setupBuffers();
    void rebuildVertexArray(Sand2D::World& world);
    void updateDirtyCells(Sand2D::World& world);
    void compileShaders();
    void updateViewport();
    
    void getColorFromId(Sand2D::ParticleId id, float& r, float& g, float& b) const;
    
    GLuint m_vao, m_vbo, m_shaderProgram;
    GLFWwindow* m_window;
    int m_windowWidth, m_windowHeight;
    int m_viewportX, m_viewportY, m_viewportWidth, m_viewportHeight;
    
    std::vector<float> m_vertices;
    std::unordered_set<int> m_dirtyCells;
    Sand2D::ParticleRegistry& m_registry;
    
    float m_scaleX, m_scaleY;
};