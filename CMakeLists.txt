cmake_minimum_required(VERSION 3.14)
project(Sand2D VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL2_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/third_party/SDL)
set(GLM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/third_party/glm)
set(FASTLZ_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/third_party/fastlz)
set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/third_party/glad)

if(EXISTS ${SDL2_DIR}/CMakeLists.txt)
    add_subdirectory(${SDL2_DIR})
else()
    find_package(SDL2 CONFIG REQUIRED)
endif()

add_subdirectory(${GLM_DIR})

add_executable(Sand2D
    src/main.cpp
    src/World/World.cpp
    src/Render/Renderer.cpp
    src/Render/Camera.cpp
    src/Render/Text.cpp
    src/Serialization/WorldSerializer.cpp
    src/Serialization/Compression.cpp
    src/Game/GameLoop.cpp
    ${FASTLZ_DIR}/fastlz.c
)

set_source_files_properties(${FASTLZ_DIR}/fastlz.c PROPERTIES LANGUAGE C)

target_link_libraries(Sand2D
    PRIVATE
    $<IF:$<TARGET_EXISTS:SDL2>,SDL2,SDL2::SDL2>
    glm::glm
)

find_package(OpenGL REQUIRED)
target_link_libraries(Sand2D PRIVATE ${OPENGL_LIBRARIES})

target_include_directories(Sand2D PRIVATE
    src
    ${FASTLZ_DIR}
    ${GLAD_DIR}/include
)

if(MSVC)
    target_compile_options(Sand2D PRIVATE
        /O2
        /arch:AVX2
        /fp:fast
        /GL
        /Oi
        /Ot
        /Oy-
    )

    target_link_options(Sand2D PRIVATE
        /LTCG
    )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Sand2D PRIVATE
        -O3
        -march=native
        -mtune=native
        -ffast-math
        -funroll-loops
        -finline-functions
        -fomit-frame-pointer
    )

    target_link_options(Sand2D PRIVATE
        -flto                    # Link-Time Optimization
    )
endif()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG")

if(WIN32 AND (MINGW OR MSVC))
    add_custom_command(TARGET Sand2D POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/libs/third_party/SDL/$<CONFIG>/SDL2$<$<CONFIG:Debug>:d>.dll"
            "$<TARGET_FILE_DIR:Sand2D>"
        COMMENT "Copying SDL2.dll..."
    )
endif()
