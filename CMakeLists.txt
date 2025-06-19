cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 20)             # available options are [98, 11, 14, 17, 20]
project(ExperiRender)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FetchContent)

# Options
option(ENABLE_NSIGHT_AFTERMATH "Enable Nsight Aftermath crash debugging integration" OFF)
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

#############
# Platform-specific build options
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 1)
    set(GLSLC_PATH "C:/VulkanSDK/1.3.283.0/Bin/glslc.exe")
elseif(UNIX AND NOT APPLE)
    # Use C++20 standard
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
    
    # Find glslc from Linux Vulkan SDK
    find_program(GLSLC_PATH glslc REQUIRED)
endif()

# Configure warnings and optionally treat them as errors
if(MSVC)
    if(WARNINGS_AS_ERRORS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
        message(STATUS "MSVC: Warnings as errors enabled")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
        message(STATUS "MSVC: High warning level enabled, warnings as errors disabled")
    endif()
else()
    if(WARNINGS_AS_ERRORS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
        message(STATUS "GCC/Clang: Warnings as errors enabled")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
        message(STATUS "GCC/Clang: High warning level enabled, warnings as errors disabled")
    endif()
endif()

# Common output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Disable C++20 modules scanning for fastgltf
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

######################################
# Dependencies
# vulkan
find_package(Vulkan REQUIRED)

# Nsight Aftermath
if(ENABLE_NSIGHT_AFTERMATH)
    find_package(NsightAftermath REQUIRED)
    add_compile_definitions(NSIGHT_AFTERMATH_ENABLED)
    message(STATUS "Nsight Aftermath is enabled")
else()
    message(STATUS "Nsight Aftermath is disabled")
endif()

# SDL2 - Platform-specific setup
if(WIN32)
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL2-2.30.3/cmake")
    find_package(SDL2 CONFIG REQUIRED)
else()
    # Linux way
    find_package(SDL2 REQUIRED)
    if(NOT SDL2_FOUND)
        message(FATAL_ERROR "SDL2 not found, install with: sudo apt install libsdl2-dev")
    endif()
endif()

# STB
add_subdirectory(deps/stb)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY    https://github.com/g-truc/glm.git
    GIT_TAG     bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
)
FetchContent_MakeAvailable(glm)

# VK-Bootstrap
FetchContent_Declare(
    Vkbootstrap
    GIT_REPOSITORY    https://github.com/charles-lunarg/vk-bootstrap.git
    GIT_TAG     50b75222b200cb6e62010573608d2e65f2e816ec #refs/tags/1.3.283
)
FetchContent_MakeAvailable(Vkbootstrap)

# VMA
FetchContent_Declare(
    VMA
    GIT_REPOSITORY    https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG     eaf8fc27eeadf6f21b11183651b829e897f01957 
)
FetchContent_MakeAvailable(VMA)

# FastGLTF
FetchContent_Declare(
    fastgltf
    GIT_REPOSITORY  https://github.com/spnda/fastgltf.git
    GIT_TAG         b5f0f79401cfc792f15f54e2c0e2c9848e8f9092
)
FetchContent_MakeAvailable(fastgltf)

# After fastgltf is made available, disable modules if target exists
if(TARGET fastgltf_module)
    set_target_properties(fastgltf_module PROPERTIES CMAKE_CXX_SCAN_FOR_MODULES OFF)
endif()

# Spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY  https://github.com/gabime/spdlog.git
    GIT_TAG         f355b3d58f7067eee1706ff3c801c2361011f3d5
)
FetchContent_MakeAvailable(spdlog)

# GTest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# IMGUI - Platform-specific setup
set(IMGUI_VERSION 1.91.8)
FetchContent_Declare (
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v${IMGUI_VERSION}
)
FetchContent_MakeAvailable(imgui)

# json
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)


file(GLOB IMGUI_SRC
    "${imgui_SOURCE_DIR}/*.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp"
)

add_library(imgui STATIC ${IMGUI_SRC})
target_include_directories(imgui PUBLIC 
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# Platform-specific linking for ImGui
if(WIN32)
    target_link_libraries(imgui Vulkan::Vulkan SDL2::SDL2)
else()
    target_link_libraries(imgui Vulkan::Vulkan ${SDL2_LIBRARIES})
    target_include_directories(imgui PUBLIC ${SDL2_INCLUDE_DIRS})
endif()

# Define paths for shaders
set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")

# Create shader output directory
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/shaders")

# List of shaders to compile
file(GLOB SHADERS
    "${SHADER_DIR}/*.frag"
    "${SHADER_DIR}/*.vert"
    "${SHADER_DIR}/*.comp"
    "${SHADER_DIR}/*.rgen"
    "${SHADER_DIR}/*.rchit"
    "${SHADER_DIR}/*.rmiss"
)

# Loop through shaders and create custom commands for each
foreach(SHADER ${SHADERS})
    set(SHADER_SRC ${SHADER})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_SPV "${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv")
    add_custom_command(
        OUTPUT ${SHADER_SPV}
        COMMAND ${GLSLC_PATH} ${SHADER_SRC} -o ${SHADER_SPV} --target-spv=spv1.6
        DEPENDS ${SHADER_SRC}
        COMMENT "Compiling shader: ${SHADER_SRC} -> ${SHADER_SPV}"
    )
    list(APPEND SPV_OUTPUTS ${SHADER_SPV})
endforeach()

# Create a target to compile all shaders
add_custom_target(compile_shaders ALL DEPENDS ${SPV_OUTPUTS})

# SubFolders
add_subdirectory(VkRenderer/Scene)

# GPU Crash Monitor - Conditional compilation based on Nsight Aftermath
if(ENABLE_NSIGHT_AFTERMATH)
    add_subdirectory(VkRenderer/GPUCrashMonitor)
endif()

# Create a library from renderer code (for testing)
file(GLOB CPP_SRC
        "VkRenderer/*.cpp"
        )
file(GLOB_RECURSE HEADERS "VkRenderer/*.h")

# Remove main.cpp from library sources if it exists
list(FILTER CPP_SRC EXCLUDE REGEX ".*main\\.cpp$")

# Create renderer library
add_library(RendererLib STATIC ${CPP_SRC} ${HEADERS})

# Platform-specific include directories for library
if(WIN32)
    target_include_directories(RendererLib PUBLIC VkRenderer/)
else()
    target_include_directories(RendererLib PUBLIC VkRenderer/ ${SDL2_INCLUDE_DIRS})
endif()

# Base libraries for renderer library
set(RENDERER_BASE_LIBS
    glm::glm 
    Vulkan::Vulkan 
    vk-bootstrap::vk-bootstrap 
    VulkanMemoryAllocator 
    imgui 
    fastgltf 
    spdlog::spdlog 
    stb 
    nlohmann_json::nlohmann_json 
    Scene
)

# Platform-specific libraries for renderer library
if(WIN32)
    target_link_libraries(RendererLib PUBLIC
        ${RENDERER_BASE_LIBS}
        SDL2::SDL2 
        SDL2::SDL2main
    )
else()
    target_link_libraries(RendererLib PUBLIC
        ${RENDERER_BASE_LIBS}
        ${SDL2_LIBRARIES}
        dl 
        pthread 
        X11
    )
endif()

# Conditionally link GPU Crash Monitor
if(ENABLE_NSIGHT_AFTERMATH)
    target_link_libraries(RendererLib PUBLIC GPUCrashMonitor)
endif()

# Main executable
if(WIN32)
    add_executable(Renderer VkRenderer/main.cpp ${SDL2_LIBRARY})
else()
    add_executable(Renderer VkRenderer/main.cpp)
endif()

# Link main executable to the library
target_link_libraries(Renderer PRIVATE RendererLib)

# Ensure the shader compilation runs before building the main target
add_dependencies(Renderer compile_shaders)
add_dependencies(RendererLib compile_shaders)

# Platform-specific post-build steps
if(WIN32)
    add_custom_command(TARGET Renderer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL2-2.30.3/lib/x64/SDL2.dll"
            $<TARGET_FILE_DIR:Renderer>)
endif()

# Enable testing
enable_testing()

# Add tests subdirectory
add_subdirectory(tests)