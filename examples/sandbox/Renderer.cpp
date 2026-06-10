#include "Renderer.h"
#include <iostream>
#include <algorithm>

const char* vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec2 aPos;
    layout(location = 1) in vec3 aColor;
    
    out vec3 vertexColor;
    uniform vec2 u_offset;
    
    void main() {
        vertexColor = aColor;
        gl_Position = vec4(aPos.x + u_offset.x, aPos.y + u_offset.y, 0.0, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    in vec3 vertexColor;
    out vec4 FragColor;
    
    void main() {
        vec3 color = vertexColor;
        color = color * 1.15;
        float gray = (color.r + color.g + color.b) / 3.0;
        color = mix(vec3(gray), color, 1.3);
        color = min(color, 1.0);
        FragColor = vec4(color, 1.0);
    }
)";

Renderer::Renderer(int width, int height, const std::string& title, Sand2D::ParticleRegistry& registry)
    : m_windowWidth(width), m_windowHeight(height),
      m_viewportX(0), m_viewportY(0), m_viewportWidth(0), m_viewportHeight(0),
      m_registry(registry)
{
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(-1);
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    
    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    
    glfwMakeContextCurrent(m_window);
    
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int w, int h) {
        Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
        renderer->updateViewport();
    });
    
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        exit(-1);
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    compileShaders();
    setupBuffers();
    updateViewport();
}

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteProgram(m_shaderProgram);
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Renderer::updateViewport() {
    glfwGetFramebufferSize(m_window, &m_windowWidth, &m_windowHeight);
    m_viewportX = 0;
    m_viewportY = 0;
    m_viewportWidth = m_windowWidth;
    m_viewportHeight = m_windowHeight;
    glViewport(m_viewportX, m_viewportY, m_viewportWidth, m_viewportHeight);
}

void Renderer::compileShaders() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex shader compilation failed:\n" << infoLog << std::endl;
    }
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed:\n" << infoLog << std::endl;
    }
    
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);
    
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed:\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Renderer::setupBuffers() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void Renderer::getColorFromId(Sand2D::ParticleId id, float& r, float& g, float& b) const {
    const auto& def = m_registry.get(id);
    uint32_t color = def.color;
    
    r = ((color >> 24) & 0xFF) / 255.0f;
    g = ((color >> 16) & 0xFF) / 255.0f;
    b = ((color >> 8) & 0xFF) / 255.0f;
}

void Renderer::rebuildVertexArray(Sand2D::World& world) {
    m_vertices.clear();
    
    float worldWidth = world.getWidth();
    float worldHeight = world.getHeight();
    m_scaleX = 2.0f / worldWidth;
    m_scaleY = 2.0f / worldHeight;
    
    for (int y = 0; y < world.getHeight(); ++y) {
        for (int x = 0; x < world.getWidth(); ++x) {
            Sand2D::ParticleId id = world.getParticleId(x, y);
            if (id == Sand2D::ParticleRegistry::Empty) continue;
            
            float r, g, b;
            getColorFromId(id, r, g, b);
            
            float px = -1.0f + x * m_scaleX;
            float py = 1.0f - y * m_scaleY;
            float pw = m_scaleX;
            float ph = m_scaleY;
            
            // Треугольник 1
            m_vertices.push_back(px); m_vertices.push_back(py);
            m_vertices.push_back(r); m_vertices.push_back(g); m_vertices.push_back(b);
            
            m_vertices.push_back(px + pw); m_vertices.push_back(py);
            m_vertices.push_back(r); m_vertices.push_back(g); m_vertices.push_back(b);
            
            m_vertices.push_back(px + pw); m_vertices.push_back(py - ph);
            m_vertices.push_back(r); m_vertices.push_back(g); m_vertices.push_back(b);
            
            // Треугольник 2
            m_vertices.push_back(px); m_vertices.push_back(py);
            m_vertices.push_back(r); m_vertices.push_back(g); m_vertices.push_back(b);
            
            m_vertices.push_back(px + pw); m_vertices.push_back(py - ph);
            m_vertices.push_back(r); m_vertices.push_back(g); m_vertices.push_back(b);
            
            m_vertices.push_back(px); m_vertices.push_back(py - ph);
            m_vertices.push_back(r); m_vertices.push_back(g); m_vertices.push_back(b);
        }
    }
}

void Renderer::render(Sand2D::World& world) {
    rebuildVertexArray(world);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(m_shaderProgram);
    
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(float), m_vertices.data(), GL_DYNAMIC_DRAW);
    
    glDrawArrays(GL_TRIANGLES, 0, m_vertices.size() / 5);
    
    glfwSwapBuffers(m_window);
}

void Renderer::handleEvents() {
    glfwPollEvents();
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_window, true);
}

void Renderer::getMouseWorldPosition(Sand2D::World& world, int& x, int& y) const {
    double mouseX, mouseY;
    glfwGetCursorPos(m_window, &mouseX, &mouseY);
    
    int windowWidth, windowHeight;
    glfwGetWindowSize(m_window, &windowWidth, &windowHeight);
    
    float worldX = (mouseX / windowWidth) * world.getWidth();
    float worldY = (mouseY / windowHeight) * world.getHeight();
    
    x = static_cast<int>(worldX);
    y = static_cast<int>(worldY);
    
    x = std::max(0, std::min(x, world.getWidth() - 1));
    y = std::max(0, std::min(y, world.getHeight() - 1));
}