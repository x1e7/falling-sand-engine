#include "UI.h"
#include "Core/ParticleRegistry.h"
#include "Serialization/WorldSerializer.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <string>

UI::UI(SDL_Window* window, SDL_Renderer* renderer) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
}

UI::~UI() {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void UI::beginFrame() {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void UI::endFrame(SDL_Renderer* renderer) {
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
}

bool UI::wantsInput() const {
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureKeyboard || io.WantCaptureMouse;
}

void UI::render(World& world, Renderer& renderer,
                bool& paused, ParticleId& currentBrush, int& brushRadius,
                int fps, int worldWidth, int worldHeight) {
    renderMainWindow(world, renderer, paused, currentBrush, brushRadius, fps, worldWidth, worldHeight);
    renderControlsWindow();
    renderDemoWindow();
}

void UI::renderMainWindow(World& world, Renderer& renderer,
                          bool& paused, ParticleId& currentBrush, int& brushRadius,
                          int fps, int worldWidth, int worldHeight) {
    ImGui::Begin("Sand2D");

    ImGui::Text("FPS: %d", fps);
    ImGui::Text("World size: %dx%d", worldWidth, worldHeight);
    ImGui::Text("Brush: %d", brushRadius);
    ImGui::Text("Paused: %s", paused ? "Yes" : "No");

    ImGui::Separator();

    const char* brushNames[] = { "Sand", "Water", "Fire", "Wall", "Oil" };

    ParticleRegistry& registry = world.getRegistry();
    ParticleId brushIds[] = {
        registry.findId("Sand"),
        registry.findId("Water"),
        registry.findId("Fire"),
        registry.findId("Wall"),
        registry.findId("Oil")
    };

    int currentIndex = 0;
    for (int i = 0; i < 5; ++i) {
        if (currentBrush == brushIds[i]) { currentIndex = i; break; }
    }

    if (ImGui::Combo("Brush", &currentIndex, brushNames, 5)) {
        currentBrush = brushIds[currentIndex];
    }

    ImGui::SliderInt("Radius", &brushRadius, 1, 128);

    ImGui::Separator();

    if (ImGui::Button("Clear")) {
        for (int y = 0; y < worldHeight; ++y) {
            for (int x = 0; x < worldWidth; ++x) {
                world.setParticle(x, y, ParticleRegistry::Empty);
            }
        }
    }

    ImGui::SameLine();

    if (ImGui::Button(paused ? "Resume" : "Pause")) {
        paused = !paused;
    }

    ImGui::Separator();

    if (ImGui::Button("Save")) {
        WorldSerializer::saveWorld(world, "world.bin");
    }

    ImGui::SameLine();

    if (ImGui::Button("Load")) {
        WorldSerializer::loadWorld(world, "world.bin");
    }

    ImGui::End();
}

void UI::renderControlsWindow() {
    ImGui::Begin("Controls");
    ImGui::Text("1-5 - Brush");
    ImGui::Text("Space - Pause");
    ImGui::Text("WASD - Move");
    ImGui::Text("Scroll - Brush size");
    ImGui::Text("RMB - Delete");
    ImGui::End();
}

void UI::renderDemoWindow() {
    if (m_showDemoWindow) {
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }
}
