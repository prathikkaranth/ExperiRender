set(SOURCE_FILES
    src/camera.cpp
    src/PointLight.cpp
    src/SceneDesc.cpp
)

file(GLOB_RECURSE HEADERS "include/*.h")

add_library(Scene STATIC ${SOURCE_FILES} ${HEADERS})

target_link_libraries(Scene PRIVATE glm spdlog SDL2::SDL2 SDL2::SDL2main nlohmann_json::nlohmann_json)

target_include_directories(Scene PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
