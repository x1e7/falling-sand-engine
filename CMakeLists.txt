cmake_minimum_required(VERSION 3.14)
project(Sand2D)

set(CMAKE_CXX_STANDARD 17)

#set(SDL2_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/third_party/SDL)

#if(EXISTS ${SDL2_DIR}/CMakeLists.txt)
#    add_subdirectory(${SDL2_DIR})
#else()
    find_package(SDL2 CONFIG REQUIRED)
#endif()

add_subdirectory(libs/third_party/glm)

add_executable(Sand2D
    src/main.cpp
    src/World/World.cpp
    src/Render/Renderer.cpp
    src/Render/Camera.cpp
    src/Render/Text.cpp
    src/Serialization/WorldSerializer.cpp
    src/Serialization/Compression.cpp
    src/Game/GameLoop.cpp
    libs/third_party/fastlz/fastlz.c
)

set_source_files_properties(libs/third_party/fastlz/fastlz.c PROPERTIES LANGUAGE C)

if(TARGET SDL2)
    target_link_libraries(Sand2D SDL2)
else()
    target_link_libraries(Sand2D SDL2::SDL2)
endif()

target_link_libraries(Sand2D glm::glm)

target_include_directories(Sand2D PRIVATE
    src
    libs/third_party/fastlz
    libs/third_party/glad/include
)

find_package(OpenGL REQUIRED)
target_link_libraries(Sand2D ${OPENGL_LIBRARIES})

if(MINGW OR MSVC)
    set(SDL2_DLL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs/third_party/SDL2/bin/SDL2.dll)
    if(EXISTS ${SDL2_DLL_PATH})
        add_custom_command(TARGET Sand2D POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${SDL2_DLL_PATH}
                $<TARGET_FILE_DIR:Sand2D>
            COMMENT "Copying SDL2.dll..."
        )
    else()
        set(SDL2_DLL_PATH ${CMAKE_BINARY_DIR}/libs/third_party/SDL2/bin/SDL2.dll)
        if(EXISTS ${SDL2_DLL_PATH})
            add_custom_command(TARGET Sand2D POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${SDL2_DLL_PATH}
                    $<TARGET_FILE_DIR:Sand2D>
                COMMENT "Copying SDL2.dll..."
            )
        endif()
    endif()
endif()
