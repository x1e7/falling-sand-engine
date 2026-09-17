set(SANDBOX_SOURCES
    World/World.cpp
    Render/Renderer.cpp
    Render/Camera.cpp
    Render/UI.cpp
    Serialization/WorldSerializer.cpp
    Serialization/Compression.cpp
    Game/GameLoop.cpp
)

add_library(sandbox_core STATIC ${SANDBOX_SOURCES})
target_include_directories(sandbox_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(sandbox_core
    PUBLIC  SDL3::SDL3 lz4::lz4
    PRIVATE imgui
)

target_compile_options(sandbox_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Release>:-O3 -funroll-loops>
        $<$<CONFIG:Debug>:-O0 -g>
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4
        $<$<CONFIG:Release>:/O2 /Oi /Ot>
    >
)

if(SANDBOX_WARNINGS_AS_ERRORS)
    target_compile_options(sandbox_core PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Werror>
        $<$<CXX_COMPILER_ID:MSVC>:/WX>
    )
endif()

if(SANDBOX_ENABLE_ASAN AND NOT MSVC)
    target_compile_options(sandbox_core PUBLIC -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(sandbox_core    PUBLIC -fsanitize=address)
endif()

add_executable(Sand2D main.cpp)
target_link_libraries(Sand2D PRIVATE sandbox_core)

if(WIN32)
    get_target_property(_sdl3_type SDL3::SDL3 TYPE)
    if(_sdl3_type STREQUAL "SHARED_LIBRARY")
        add_custom_command(TARGET Sand2D POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:Sand2D>
            COMMENT "Copying SDL3.dll")
    endif()

    if(MINGW)
        set(MINGW_PREFIX "C:/msys64/ucrt64" CACHE PATH "MinGW prefix")
        foreach(_dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
            set(_src "${MINGW_PREFIX}/bin/${_dll}")
            if(EXISTS "${_src}")
                add_custom_command(TARGET Sand2D POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_src}" "$<TARGET_FILE_DIR:Sand2D>")
            else()
                message(WARNING "[sandbox] Not found: ${_src}")
            endif()
        endforeach()
    endif()
endif()
