cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 20)             # available options are [98, 11, 14, 17. 20]

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 1) # TODO is this a hack?
project(ExperiRender)

include(FetchContent)

#############
# Build options
if(MSVC)
    set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

######################################
# Dependencies

# vulkan
find_package(Vulkan REQUIRED)

# SDL
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL2-2.30.3/cmake")
find_package(SDL2 CONFIG REQUIRED)

# GLFW
add_subdirectory(deps/glfw-3.4)

# GLM
FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm.git
	GIT_TAG 	bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
)
FetchContent_MakeAvailable(glm)

# VK-Bootstrap
FetchContent_Declare(
	Vkbootstrap
	GIT_REPOSITORY	https://github.com/charles-lunarg/vk-bootstrap.git
	GIT_TAG 	50b75222b200cb6e62010573608d2e65f2e816ec #refs/tags/1.3.283
)
FetchContent_MakeAvailable(Vkbootstrap)

# VMA
FetchContent_Declare(
	VMA
	GIT_REPOSITORY	https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
	GIT_TAG 	eaf8fc27eeadf6f21b11183651b829e897f01957 
)
FetchContent_MakeAvailable(VMA)



# FastGLTF
FetchContent_Declare(
    fastgltf
    GIT_REPOSITORY  https://github.com/spnda/fastgltf.git
    GIT_TAG         b5f0f79401cfc792f15f54e2c0e2c9848e8f9092
)
FetchContent_MakeAvailable(fastgltf)

# Spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY  https://github.com/gabime/spdlog.git
    GIT_TAG         f355b3d58f7067eee1706ff3c801c2361011f3d5
)
FetchContent_MakeAvailable(spdlog)

# IMGUI
file(GLOB IMGUI_SRC
    "deps/imgui/*.cpp"
    "deps/imgui/backends/imgui_impl_vulkan.cpp"
    "deps/imgui/backends/imgui_impl_sdl2.cpp"
)

add_library(imgui STATIC ${IMGUI_SRC})
target_link_libraries(imgui Vulkan::Vulkan SDL2::SDL2)
target_include_directories(imgui PUBLIC deps/imgui)

# Define paths for shaders
set(GLSLC_PATH "C:/VulkanSDK/1.3.283.0/Bin/glslc.exe")
set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")

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
# TODO Shader dependencies
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

# source and headers
file(GLOB CPP_SRC
        "VkRenderer/*.cpp"
        )
file(GLOB_RECURSE HEADERS "VkRenderer/*.h")

add_executable(Renderer ${CPP_SRC} ${HEADERS} ${SDL2_LIBRARY})

# Ensure the shader compilation runs before building the main target
add_dependencies(Renderer compile_shaders)

target_include_directories(Renderer PRIVATE VkRenderer/)

# External Dependencies
target_link_libraries(Renderer PRIVATE
    glm::glm glfw Vulkan::Vulkan SDL2::SDL2 SDL2::SDL2main vk-bootstrap::vk-bootstrap VulkanMemoryAllocator imgui fastgltf spdlog::spdlog)

# Internal Dependencies
target_link_libraries(Renderer PRIVATE Scene)


if (WIN32)
    add_custom_command(TARGET Renderer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL2-2.30.3/lib/x64/SDL2.dll"
            $<TARGET_FILE_DIR:Renderer>)
endif()

