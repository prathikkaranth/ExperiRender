// SceneDesc.h
#pragma once

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>
#include <string>

#include "glm/fwd.hpp"

class SceneDesc {
public:
    // Structure to hold scene information
    struct SceneInfo {
        std::string name;
        std::string filePath;

        bool hasTransform;
        glm::vec3 translate = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
        glm::vec3 rotate = glm::vec3(0.0f); // Euler angles in degrees
    };

    // Structure for HDRI information
    struct HDRIInfo {
        std::string filePath;
    };

    // Static method to get information for all scenes from a JSON file
    static std::vector<SceneInfo> getAllScenes(const std::string& jsonFilePath);

    // Static method to get scene information from a JSON file
    static SceneInfo getSceneInfo(const std::string& jsonFilePath);

    // Static method for HDRI
    static HDRIInfo getHDRIInfo(const std::string& jsonFilePath);
};