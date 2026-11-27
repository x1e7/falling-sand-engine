#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "World/World.h"
#include "Render/Renderer.h"
#include "Render/Camera.h"
#include "UI/UIRenderer.h"
#include "UI/UICanvas.h"
#include "UI/Label.h"
#include "UI/Button.h"

class GameLoop {
public:
    GameLoop();
    ~GameLoop();

    void run();
    void paintBrush(int wx, int wy, ParticleId id);

private:
    void handleInput(float deltaTime);
    void render();
    void refreshBrushButtons();
    void setBrush(ParticleId id, const std::string& name);
    void togglePause();
    void saveWorld();
    void updateBrushLabel();
    void updateDiagnostics(Uint64 frameStart, Uint64 t1, Uint64 t2, Uint64 t3, Uint64 t4, float deltaTime);

    Label* addLabel(float x, float y, float w, const char* text);
    ParticleId cachedId(const char* name);

    ParticleRegistry m_registry;
    std::unique_ptr<World> m_world;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<UIRenderer> m_uiRenderer;
    std::unique_ptr<UICanvas> m_uiCanvas;

    Label* m_fpsLabel = nullptr;
    Label* m_totalLabel = nullptr;
    Label* m_simLabel = nullptr;
    Label* m_renderLabel = nullptr;
    Label* m_chunksLabel = nullptr;
    Label* m_brushLabel = nullptr;
    Button* m_pauseBtn = nullptr;

    struct BrushButton {
        Button* button;
        ParticleId id;
    };
    std::vector<BrushButton> m_brushButtons;
    std::unordered_map<std::string, ParticleId> m_idCache;
    ParticleId m_smokeId = ParticleRegistry::Empty;

    bool m_running = true;
    bool m_paused = false;
    ParticleId m_currentBrush = ParticleRegistry::Empty;
    std::string m_currentBrushName = "Sand";
    int m_brushRadius = 1;

    int m_fps = 0;
    int m_frameCount = 0;
    float m_fpsTimer = 0.0f;
    Uint32 m_lastTime = 0;

    float m_msTotal = 0.0f;
    float m_msSim = 0.0f;
    float m_msRender = 0.0f;
    float m_msPresent = 0.0f;
    float m_usPerTick = 0.0f;
    int m_activeChunks = 0;
    int m_totalChunks = 0;

    Uint64 m_sumSim = 0;
    Uint64 m_sumRender = 0;
    Uint64 m_sumPresent = 0;
    Uint64 m_sumTotal = 0;
    Uint64 m_sumTicks = 0;
    int m_diagFrames = 0;
    Uint64 m_diagStart = 0;
    Uint64 m_perfFreq = 0;

    float m_autosaveTimer = 0.0f;
    std::string m_savePath;
    static constexpr float AUTOSAVE_INTERVAL = 60.0f;
};
