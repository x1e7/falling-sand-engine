find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)

set(SANDBOX_SOURCES
    World/World.cpp
    Render/Renderer.cpp
    Render/Camera.cpp
    UI/UIRenderer.cpp
    UI/UICanvas.cpp
    Serialization/WorldSerializer.cpp
    Serialization/Compression.cpp
    Game/GameLoop.cpp
)

add_library(sandbox_core STATIC ${SANDBOX_SOURCES})
target_include_directories(sandbox_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(sandbox_core
    PUBLIC  SDL3::SDL3 SDL3_ttf::SDL3_ttf lz4::lz4
)

add_executable(Sand2D main.cpp)
target_link_libraries(Sand2D PRIVATE sandbox_core)
