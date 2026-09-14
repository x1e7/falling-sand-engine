include(FetchContent)

if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

function(sandbox_add_dependency NAME)
    cmake_parse_arguments(ARG
        "NO_CMAKE"
        "GIT_REPO;GIT_TAG;SUBDIR;LOCAL_DIR"
        "OPTIONS"
        ${ARGN}
    )

    if(ARG_LOCAL_DIR)
        set(_local_dir "${CMAKE_SOURCE_DIR}/${ARG_LOCAL_DIR}")
    else()
        set(_local_dir "${CMAKE_SOURCE_DIR}/third_party/${NAME}")
    endif()

    foreach(_opt IN LISTS ARG_OPTIONS)
        string(REPLACE "=" ";" _kv "${_opt}")
        list(GET _kv 0 _key)
        list(GET _kv 1 _val)
        set(${_key} "${_val}" CACHE BOOL "" FORCE)
    endforeach()

    if(EXISTS "${_local_dir}")
        if(NOT ARG_NO_CMAKE AND EXISTS "${_local_dir}/CMakeLists.txt")
            add_subdirectory("${_local_dir}" "${CMAKE_BINARY_DIR}/_deps/${NAME}-build")
        endif()
        set(${NAME}_SOURCE_DIR "${_local_dir}" PARENT_SCOPE)
        return()
    endif()

    if(NOT ARG_GIT_REPO)
        message(FATAL_ERROR
            "[sandbox] ${NAME}: not found at ${_local_dir} and no GIT_REPO"
        )
    endif()

    set(_declare_args
        GIT_REPOSITORY "${ARG_GIT_REPO}"
        GIT_TAG "${ARG_GIT_TAG}"
        GIT_SHALLOW TRUE
    )
    if(ARG_SUBDIR)
        list(APPEND _declare_args SOURCE_SUBDIR "${ARG_SUBDIR}")
    endif()

    FetchContent_Declare(${NAME} ${_declare_args})

    if(ARG_NO_CMAKE)
        FetchContent_GetProperties(${NAME})
        if(NOT ${NAME}_POPULATED)
            FetchContent_Populate(${NAME})
        endif()
    else()
        FetchContent_MakeAvailable(${NAME})
    endif()

    set(${NAME}_SOURCE_DIR "${${NAME}_SOURCE_DIR}" PARENT_SCOPE)
endfunction()

sandbox_add_dependency(SDL3
    LOCAL_DIR third_party/SDL3-3.4.14
    GIT_REPO  https://github.com/libsdl-org/SDL.git
    GIT_TAG   release-3.4.14
    OPTIONS
        "SDL_SHARED=ON"
        "SDL_STATIC=OFF"
        "SDL_TEST=OFF"
)

sandbox_add_dependency(imgui
    GIT_REPO https://github.com/ocornut/imgui.git
    GIT_TAG  docking
    NO_CMAKE
)

if(NOT TARGET imgui)
    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
    )
    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(imgui PUBLIC SDL3::SDL3)
endif()

if(NOT TARGET lz4::lz4)
    set(_lz4_dir "${CMAKE_SOURCE_DIR}/third_party/lz4")
    if(NOT EXISTS "${_lz4_dir}/lz4.c")
        set(_lz4_dir "${CMAKE_SOURCE_DIR}/third_party/lz4/lib")
    endif()
    add_library(lz4 STATIC
        ${_lz4_dir}/lz4.c
        ${_lz4_dir}/lz4hc.c
        ${_lz4_dir}/lz4frame.c
        ${_lz4_dir}/xxhash.c
    )
    target_include_directories(lz4 PUBLIC ${_lz4_dir})
    add_library(lz4::lz4 ALIAS lz4)
endif()
