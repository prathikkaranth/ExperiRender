cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 17)             # available options are [98, 11, 14, 17. 20]

project(RendererPK)

include(FetchContent)

######################################
# Dependencies

# vulkan
find_package(Vulkan REQUIRED)

# SDL
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL2-2.0.14/cmake")
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

# GLM
FetchContent_Declare(
	Vkbootstrap
	GIT_REPOSITORY	https://github.com/charles-lunarg/vk-bootstrap.git
	GIT_TAG 	50b75222b200cb6e62010573608d2e65f2e816ec #refs/tags/1.3.283
)
FetchContent_MakeAvailable(Vkbootstrap)

# source and headers
file(GLOB CPP_SRC
        "src/*.cpp"
        )
file(GLOB_RECURSE HEADERS "src/*.h")

add_executable(Renderer ${CPP_SRC} ${HEADERS} ${SDL2_LIBRARY})
target_include_directories(Renderer PRIVATE src/)
target_link_libraries(Renderer PRIVATE glm::glm glfw Vulkan::Vulkan SDL2::SDL2 SDL2::SDL2main vk-bootstrap::vk-bootstrap)

if (WIN32)
    add_custom_command(TARGET Renderer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/deps/SDL2-2.30.3/lib/x64/SDL2.dll"
            $<TARGET_FILE_DIR:Renderer>)
endif()