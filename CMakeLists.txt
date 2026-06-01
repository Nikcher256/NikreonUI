cmake_minimum_required(VERSION 3.24)

project(NikreonUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(GLSLC_EXECUTABLE glslc HINTS "$ENV{VULKAN_SDK}/Bin")
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc was not found. Install the Vulkan SDK so NikreonUI shaders can be compiled.")
endif()

find_package(glm CONFIG REQUIRED)
find_package(Freetype REQUIRED)
find_package(volk CONFIG REQUIRED)

set(NIKREON_UI_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
set(NIKREON_UI_SHADERS
    renderer2d_quad.vert
    renderer2d_quad.frag
    renderer2d_sdf_rect.vert
    renderer2d_sdf_rect.frag
    text.vert
    text.frag
)

set(NIKREON_UI_COMPILED_SHADERS)
foreach(SHADER ${NIKREON_UI_SHADERS})
    set(SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER}")
    set(SHADER_OUTPUT "${NIKREON_UI_SHADER_OUTPUT_DIR}/${SHADER}.spv")
    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${NIKREON_UI_SHADER_OUTPUT_DIR}"
        COMMAND "${GLSLC_EXECUTABLE}" "${SHADER_SOURCE}" -o "${SHADER_OUTPUT}"
        DEPENDS "${SHADER_SOURCE}"
        VERBATIM
    )
    list(APPEND NIKREON_UI_COMPILED_SHADERS "${SHADER_OUTPUT}")
endforeach()

add_custom_target(NikreonUIShaders DEPENDS ${NIKREON_UI_COMPILED_SHADERS})

add_library(NikreonUI STATIC
    src/Engine/Renderer/Vulkan/VulkanRenderer2D.cpp
    src/Engine/Renderer/Vulkan/VulkanTextRenderer.cpp
    src/Engine/UI/Button.cpp
    src/Engine/UI/Checkbox.cpp
    src/Engine/UI/Layout.cpp
    src/Engine/UI/Slider.cpp
    src/Engine/UI/UIContext.cpp
    src/Engine/UI/UIStyleParser.cpp
    src/Engine/UI/Widget.cpp
)
add_library(NikreonUI::NikreonUI ALIAS NikreonUI)

add_dependencies(NikreonUI NikreonUIShaders)

target_include_directories(NikreonUI PUBLIC include)
target_link_libraries(NikreonUI PUBLIC glm::glm volk::volk volk::volk_headers Freetype::Freetype)
target_compile_definitions(NikreonUI PRIVATE NIKREON_SHADER_DIR="${NIKREON_UI_SHADER_OUTPUT_DIR}")

if(MSVC)
    target_compile_options(NikreonUI PRIVATE /W4 /permissive- /utf-8 /EHsc)
else()
    target_compile_options(NikreonUI PRIVATE -Wall -Wextra -Wpedantic)
endif()
