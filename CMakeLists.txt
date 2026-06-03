cmake_minimum_required(VERSION 3.24)

project(NikreonUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NIKREON_UI_BUILD_VULKAN "Build the Vulkan rendering backend" ON)
option(NIKREON_UI_BUILD_SAMPLE "Build the standalone headless sample" ON)
option(NIKREON_UI_BUILD_TESTS "Build focused widget tests" ON)

find_package(glm CONFIG REQUIRED)

add_library(NikreonUICore STATIC
    src/Engine/UI/BasicWidgets.cpp
    src/Engine/UI/Button.cpp
    src/Engine/UI/Checkbox.cpp
    src/Engine/UI/ColorPicker.cpp
    src/Engine/UI/FilePathInput.cpp
    src/Engine/UI/Layout.cpp
    src/Engine/UI/NumberInput.cpp
    src/Engine/UI/ScrollContainer.cpp
    src/Engine/UI/Slider.cpp
    src/Engine/UI/TextInput.cpp
    src/Engine/UI/UIContext.cpp
    src/Engine/UI/UIFrame.cpp
    src/Engine/UI/UIStyleParser.cpp
    src/Engine/UI/Widget.cpp
)
add_library(NikreonUI::Core ALIAS NikreonUICore)
target_include_directories(NikreonUICore PUBLIC include)
target_link_libraries(NikreonUICore PUBLIC glm::glm)

if(NIKREON_UI_BUILD_VULKAN)
    find_program(GLSLC_EXECUTABLE glslc HINTS "$ENV{VULKAN_SDK}/Bin")
    if(NOT GLSLC_EXECUTABLE)
        message(FATAL_ERROR "glslc was not found. Install the Vulkan SDK or configure with -DNIKREON_UI_BUILD_VULKAN=OFF.")
    endif()
    find_package(Freetype REQUIRED)
    find_package(volk CONFIG REQUIRED)
    set(NIKREON_UI_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    set(NIKREON_UI_SHADERS renderer2d_quad.vert renderer2d_quad.frag renderer2d_sdf_rect.vert renderer2d_sdf_rect.frag text.vert text.frag)
    foreach(SHADER ${NIKREON_UI_SHADERS})
        set(SHADER_OUTPUT "${NIKREON_UI_SHADER_OUTPUT_DIR}/${SHADER}.spv")
        add_custom_command(OUTPUT "${SHADER_OUTPUT}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${NIKREON_UI_SHADER_OUTPUT_DIR}"
            COMMAND "${GLSLC_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER}" -o "${SHADER_OUTPUT}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER}" VERBATIM)
        list(APPEND NIKREON_UI_COMPILED_SHADERS "${SHADER_OUTPUT}")
    endforeach()
    add_custom_target(NikreonUIShaders DEPENDS ${NIKREON_UI_COMPILED_SHADERS})
    add_library(NikreonUIVulkan STATIC src/Engine/Renderer/Vulkan/VulkanRenderer2D.cpp src/Engine/Renderer/Vulkan/VulkanTextRenderer.cpp)
    add_library(NikreonUI::Vulkan ALIAS NikreonUIVulkan)
    add_library(NikreonUI::NikreonUI ALIAS NikreonUIVulkan)
    add_dependencies(NikreonUIVulkan NikreonUIShaders)
    target_include_directories(NikreonUIVulkan PUBLIC include)
    target_link_libraries(NikreonUIVulkan PUBLIC NikreonUI::Core volk::volk volk::volk_headers Freetype::Freetype)
    target_compile_definitions(NikreonUIVulkan PRIVATE NIKREON_SHADER_DIR="${NIKREON_UI_SHADER_OUTPUT_DIR}")
endif()

if(NIKREON_UI_BUILD_SAMPLE)
    add_executable(NikreonUISample samples/main.cpp)
    target_link_libraries(NikreonUISample PRIVATE NikreonUI::Core)
endif()

if(NIKREON_UI_BUILD_TESTS)
    enable_testing()
    add_executable(NikreonUIWidgetTests tests/widget_tests.cpp)
    target_link_libraries(NikreonUIWidgetTests PRIVATE NikreonUI::Core)
    add_test(NAME NikreonUIWidgetTests COMMAND NikreonUIWidgetTests)
endif()

foreach(TARGET NikreonUICore NikreonUIVulkan NikreonUISample NikreonUIWidgetTests)
    if(TARGET ${TARGET})
        if(MSVC)
            target_compile_options(${TARGET} PRIVATE /W4 /permissive- /utf-8 /EHsc)
        else()
            target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endif()
endforeach()
