cmake_minimum_required(VERSION 3.10)
project(Sand2D VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

enable_language(C)

add_library(Sand2D STATIC
    src/World/World.cpp
    src/Physics/PhysicsSystem.cpp
    src/Serialization/WorldSerializer.cpp
    src/Serialization/Compression.cpp
    include/Sand2D/third_party/fastlz/fastlz.c
)

set_source_files_properties(
    include/Sand2D/third_party/fastlz/fastlz.c
    PROPERTIES LANGUAGE C
)

target_include_directories(Sand2D
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Sand2D/third_party
)

if(MSVC)
    set_source_files_properties(
        include/Sand2D/third_party/fastlz.c
        PROPERTIES COMPILE_FLAGS "/wd4100"
    )

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /Ot /Oi /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(
        include/Sand2D/third_party/fastlz/fastlz.c
        PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter -Wno-missing-field-initializers"
    )

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -mtune=native")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math -funroll-loops")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fomit-frame-pointer -fno-rtti")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif()

find_package(SDL2 CONFIG REQUIRED)

add_executable(FallingSandExample
    examples/sandbox/main.cpp
    examples/sandbox/Render/Renderer.cpp
)

target_link_libraries(FallingSandExample
    Sand2D
    SDL2::SDL2
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(FallingSandExample PRIVATE
        -O3
        -march=native
        -mtune=native
        -ffast-math
        -funroll-loops
        -fomit-frame-pointer
        -fno-exceptions
        -fno-rtti
    )
endif()

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
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
endif()
