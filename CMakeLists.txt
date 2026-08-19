cmake_minimum_required(VERSION 3.10)
project(Sand2D VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

enable_language(C)

add_library(Sand2D STATIC
    src/World/World.cpp
    src/Serialization/WorldSerializer.cpp
    src/Serialization/Compression.cpp
    libs/third_party/fastlz/fastlz.c
)

set_source_files_properties(
    libs/third_party/fastlz/fastlz.c
    PROPERTIES LANGUAGE C
)

target_include_directories(Sand2D
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/third_party/fastlz
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(FallingSandExample
    src/main.cpp
    src/Render/Renderer.cpp
    src/Render/Text.cpp
    src/Render/Camera.cpp
    src/Game/GameLoop.cpp
)

target_link_libraries(FallingSandExample
    Sand2D
    SDL2::SDL2
)

target_include_directories(FallingSandExample
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
    set_source_files_properties(
        libs/third_party/fastlz/fastlz.c
        PROPERTIES COMPILE_FLAGS "/wd4100"
    )

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /Ot /Oi /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(
        libs/third_party/fastlz/fastlz.c
        PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter -Wno-missing-field-initializers"
    )

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -mtune=native")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math -funroll-loops")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fomit-frame-pointer -fno-rtti")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif()

find_package(SDL2 CONFIG REQUIRED)

if(WIN32)
    add_custom_command(TARGET FallingSandExample POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:FallingSandExample>
            $<TARGET_FILE_DIR:FallingSandExample>
        COMMAND_EXPAND_LISTS
    )
endif()

install(TARGETS Sand2D DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER_ID}")
