#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Sand2D/World/World.h>
#include <string>
#include <vector>

class Renderer {
public:
    Renderer(int width, int height, const std::string& title, Sand2D::ParticleRegistry& registry);
    ~Renderer();

    void render(Sand2D::World& world);
    void handleEvents();

    bool isOpen() const { return !glfwWindowShouldClose(m_window); }
    GLFWwindow* getWindow() { return m_window; }
    void getMouseWorldPosition(Sand2D::World& world, int& x, int& y) const;

private:
    void setupQuad();
    void setupTexture(int width, int height);
    void updateTexture(Sand2D::World& world);
    void compileShaders();
    void updateViewport();
    uint32_t getColor(Sand2D::ParticleId id) const;

    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;
    GLuint m_texture;
    GLuint m_shaderProgram;

    GLFWwindow* m_window;
    int m_windowWidth;
    int m_windowHeight;
    int m_viewportX, m_viewportY, m_viewportWidth, m_viewportHeight;

    std::vector<uint32_t> m_pixelBuffer;
    Sand2D::ParticleRegistry& m_registry;
};
