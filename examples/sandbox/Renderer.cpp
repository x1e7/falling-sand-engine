#include "Renderer.h"
#include <iostream>
#include <algorithm>

const char* vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec2 aPos;
    layout(location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

    void main() {
        TexCoord = aTexCoord;
        gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    in vec2 TexCoord;
    out vec4 FragColor;

    uniform sampler2D worldTexture;

    void main() {
        vec4 color = texture(worldTexture, TexCoord);
        FragColor = color;
    }
)";

Renderer::Renderer(int width, int height, const std::string& title, Sand2D::ParticleRegistry& registry)
    : m_windowWidth(width)
    , m_windowHeight(height)
    , m_viewportX(0)
    , m_viewportY(0)
    , m_viewportWidth(0)
    , m_viewportHeight(0)
    , m_registry(registry)
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
    setupTexture(width, height);
    setupQuad();
    updateViewport();
}

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
    glDeleteTextures(1, &m_texture);
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

void Renderer::setupTexture(int width, int height) {
    m_pixelBuffer.resize(static_cast<size_t>(width) * height);

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::setupQuad() {
    float vertices[] = {
        -1.0f, -1.0f,     0.0f, 0.0f,
         1.0f, -1.0f,     1.0f, 0.0f,
         1.0f,  1.0f,     1.0f, 1.0f,
        -1.0f,  1.0f,     0.0f, 1.0f
    };

    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::updateTexture(Sand2D::World& world) {
    const int width = world.getWidth();
    const int height = world.getHeight();
    const auto& registry = world.getRegistry();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Sand2D::ParticleId id = world.getParticleId(x, height - 1 - y);
            uint32_t color;

            if (id == Sand2D::ParticleRegistry::Empty) {
                color = 0x00000000;
            } else {
                uint32_t rawColor = registry.get(id).color;

                uint8_t r = (rawColor >> 24) & 0xFF;
                uint8_t g = (rawColor >> 16) & 0xFF;
                uint8_t b = (rawColor >> 8) & 0xFF;
                uint8_t a = rawColor & 0xFF;

                color = (a << 24) | (b << 16) | (g << 8) | r;
            }

            m_pixelBuffer[static_cast<size_t>(y) * width + x] = color;
        }
    }

    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0, 0,
        width,
        height,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        m_pixelBuffer.data()
    );
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::render(Sand2D::World& world) {
    updateTexture(world);

    glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_shaderProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    GLint textureLoc = glGetUniformLocation(m_shaderProgram, "worldTexture");
    glUniform1i(textureLoc, 0);

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(m_window);
}

void Renderer::handleEvents() {
    glfwPollEvents();
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, true);
    }
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
