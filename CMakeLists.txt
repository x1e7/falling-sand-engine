cmake_minimum_required(VERSION 3.10)
project(Sand2D VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(Sand2D STATIC
    src/World.cpp
    src/PhysicsSystem.cpp
)

target_include_directories(Sand2D PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

add_executable(FallingSandExample
    examples/sandbox/main.cpp
    examples/sandbox/Renderer.cpp
)

target_link_libraries(FallingSandExample
    Sand2D
    OpenGL::GL
    glfw
    GLEW::GLEW
)

install(TARGETS Sand2D DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)