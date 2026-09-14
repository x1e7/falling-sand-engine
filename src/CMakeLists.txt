file(GLOB_RECURSE SANDBOX_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)
list(REMOVE_ITEM SANDBOX_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

add_library(sandbox_core STATIC ${SANDBOX_SOURCES})

target_include_directories(sandbox_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(sandbox_core
    PUBLIC
        SDL3::SDL3
        lz4::lz4
    PRIVATE
        imgui
)

target_compile_options(sandbox_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
        -Wall -Wextra -Wpedantic
    >
)

if(SANDBOX_WARNINGS_AS_ERRORS)
    target_compile_options(sandbox_core PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Werror>
    )
endif()

if(SANDBOX_ENABLE_ASAN AND NOT MSVC)
    target_compile_options(sandbox_core PUBLIC -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(sandbox_core PUBLIC -fsanitize=address)
endif()

add_executable(Sand2D main.cpp)
target_link_libraries(Sand2D PRIVATE sandbox_core)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Sand2D PRIVATE -O3 -mavx2 -ffast-math -funroll-loops)
endif()

if(MSVC)
    target_compile_options(Sand2D PRIVATE /O2 /arch:AVX2 /fp:fast /Oi /Ot)
endif()

if(WIN32)
    get_target_property(_sdl3_type SDL3::SDL3 TYPE)
    if(_sdl3_type STREQUAL "SHARED_LIBRARY")
        add_custom_command(TARGET Sand2D POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:SDL3::SDL3>
                $<TARGET_FILE_DIR:Sand2D>
            COMMENT "Copying SDL3.dll"
        )
    endif()

    if(MINGW)
        if(NOT MINGW_PREFIX)
            set(MINGW_PREFIX "C:/msys64/ucrt64")
        endif()

        message(STATUS "[sandbox] MINGW_PREFIX = ${MINGW_PREFIX}")

        foreach(_dll
            libgcc_s_seh-1.dll
            libstdc++-6.dll
            libwinpthread-1.dll
        )
            set(_src "${MINGW_PREFIX}/bin/${_dll}")
            if(EXISTS "${_src}")
                message(STATUS "[sandbox] Copying ${_dll}")
                add_custom_command(TARGET Sand2D POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_src}"
                        "$<TARGET_FILE_DIR:Sand2D>"
                )
            else()
                message(WARNING "[sandbox] Not found: ${_src}")
            endif()
        endforeach()
    endif()
endif()
