cmake_minimum_required(VERSION 3.14)
project(Sand2D VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL3_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL3-3.4.14)
set(FASTLZ_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fastlz)
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)

find_package(OpenGL REQUIRED)

if(EXISTS ${SDL3_DIR}/CMakeLists.txt)
    add_subdirectory(${SDL3_DIR})
endif()

set(IMGUI_SOURCES
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
  ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

add_executable(Sand2D
  src/main.cpp
  src/World/World.cpp
  src/Render/Renderer.cpp
  src/Render/Camera.cpp
  src/Render/UI.cpp
  src/Serialization/WorldSerializer.cpp
  src/Serialization/Compression.cpp
  src/Game/GameLoop.cpp
  ${FASTLZ_DIR}/fastlz.c
  ${IMGUI_SOURCES}
)

set_source_files_properties(${FASTLZ_DIR}/fastlz.c PROPERTIES LANGUAGE C)

target_include_directories(Sand2D PRIVATE
  src
  ${FASTLZ_DIR}
  ${SDL3_DIR}/include
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)

target_link_libraries(Sand2D
  PRIVATE
  $<IF:$<TARGET_EXISTS:SDL3>,SDL3,SDL3::SDL3>
  ${OPENGL_LIBRARIES}
)

if(MSVC)
  target_compile_options(Sand2D PRIVATE
    /O2 /arch:AVX2 /fp:fast /GL /Oi /Ot /Oy-
  )
  target_link_options(Sand2D PRIVATE /LTCG)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(Sand2D PRIVATE
    -O3 -march=native -mtune=native -ffast-math
    -funroll-loops -finline-functions -fomit-frame-pointer
  )
  target_link_options(Sand2D PRIVATE -flto)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG")

if(WIN32 AND (MINGW OR MSVC))
  set(SDL3_DLL_PATH "")
  # --- SDL3.dll search paths ---
  if(EXISTS "${CMAKE_BINARY_DIR}/third_party/SDL3-3.4.14/Release/SDL3.dll")
    set(SDL3_DLL_PATH
    "${CMAKE_BINARY_DIR}/third_party/SDL3-3.4.14/Release/SDL3.dll"
  )
  endif()
  if(NOT SDL3_DLL_PATH AND EXISTS "${SDL3_DIR}/build/Release/SDL3.dll")
    set(SDL3_DLL_PATH "${SDL3_DIR}/build/Release/SDL3.dll")
  endif()
  if(NOT SDL3_DLL_PATH AND EXISTS "${CMAKE_BINARY_DIR}/bin/Release/SDL3.dll")
    set(SDL3_DLL_PATH "${CMAKE_BINARY_DIR}/bin/Release/SDL3.dll")
  endif()
  if(NOT SDL3_DLL_PATH AND EXISTS "${CMAKE_BINARY_DIR}/lib/Release/SDL3.dll")
    set(SDL3_DLL_PATH "${CMAKE_BINARY_DIR}/lib/Release/SDL3.dll")
  endif()
  if(NOT SDL3_DLL_PATH AND EXISTS "${CMAKE_BINARY_DIR}/third_party/SDL3-3.4.14/SDL3.dll")
    set(SDL3_DLL_PATH "${CMAKE_BINARY_DIR}/third_party/SDL3-3.4.14/SDL3.dll")
  endif()
  if(SDL3_DLL_PATH AND EXISTS ${SDL3_DLL_PATH})
    message(STATUS "Found SDL3.dll at: ${SDL3_DLL_PATH}")
    add_custom_command(TARGET Sand2D POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
        "${SDL3_DLL_PATH}"
        "$<TARGET_FILE_DIR:Sand2D>/SDL3.dll"
      COMMENT "Copying SDL3.dll from ${SDL3_DLL_PATH}..."
      DEPENDS SDL3
    )
  else()
    message(WARNING "SDL3.dll not found! Search paths:")
    message(WARNING " ${CMAKE_BINARY_DIR}/third_party/SDL3-3.4.14/Release/SDL3.dll")
    message(WARNING " ${SDL3_DIR}/build/Release/SDL3.dll")
    message(WARNING " ${CMAKE_BINARY_DIR}/third_party/SDL3-3.4.14/SDL3.dll")
    message(WARNING "Please copy SDL3.dll manually to build/")
  endif()
endif()
