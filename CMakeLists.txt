cmake_minimum_required(VERSION 3.14)
project(Sand2D VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL3_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL)
set(FASTLZ_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fastlz)

if(EXISTS ${SDL3_DIR}/CMakeLists.txt)
    add_subdirectory(${SDL3_DIR})
else()
    find_package(SDL3 CONFIG REQUIRED)
endif()

add_executable(Sand2D
    src/main.cpp
    src/World/World.cpp
    src/Render/Renderer.cpp
    src/Render/Camera.cpp
    src/Serialization/WorldSerializer.cpp
    src/Serialization/Compression.cpp
    src/Game/GameLoop.cpp
    ${FASTLZ_DIR}/fastlz.c
)

set_source_files_properties(${FASTLZ_DIR}/fastlz.c PROPERTIES LANGUAGE C)

target_include_directories(Sand2D PRIVATE
    src
    ${FASTLZ_DIR}
    ${SDL3_DIR}/include
)

target_link_libraries(Sand2D
    PRIVATE
    $<IF:$<TARGET_EXISTS:SDL3>,SDL3,SDL3::SDL3>
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
    target_link_options(Sand2D PRIVATE /LTCG)
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
    target_link_options(Sand2D PRIVATE -flto)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG")

if(WIN32 AND (MINGW OR MSVC))
    set(SDL3_DLL_PATH "${CMAKE_BINARY_DIR}/third_party/SDL/Release/SDL3.dll")

    if(EXISTS ${SDL3_DLL_PATH})
        add_custom_command(TARGET Sand2D POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${SDL3_DLL_PATH}"
                "$<TARGET_FILE_DIR:Sand2D>/SDL3.dll"
            COMMENT "Copying SDL3.dll from ${SDL3_DLL_PATH}..."
        )
    else()
        message(WARNING "SDL3.dll not found at ${SDL3_DLL_PATH}")
        message(WARNING "Please copy manually: copy build\\third_party\\SDL\\Release\\SDL3.dll build\\Release\\")
    endif()
endif()
