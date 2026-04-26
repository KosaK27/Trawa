cmake_minimum_required(VERSION 3.20)
project(Trawa)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
        src/Main.cpp
)

set(HEADER_FILES
        src/blades.hpp
        src/camera.hpp
        src/geometry.hpp
        src/nurbs.hpp
        src/shader_utils.hpp
        src/Shadows.hpp
        src/Wind.hpp
)

set(IMGUI_SOURCES
        external/imgui/imgui.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/imgui_widgets.cpp
        external/imgui/imgui_tables.cpp
        external/imgui/backends/imgui_impl_opengl3.cpp
        external/imgui/backends/imgui_impl_glfw.cpp
)

set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ONLY_LIBS ON CACHE BOOL "" FORCE)
add_subdirectory(external/glew)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
add_subdirectory(external/glfw)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADER_FILES} ${IMGUI_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
        external/glfw/include
        external/glew/include
        external/glm
        external/imgui
        external/imgui/backends
        src
)

target_compile_definitions(${PROJECT_NAME} PRIVATE GLEW_STATIC)

set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        libglew_static
        OpenGL::GL
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE OFF)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()