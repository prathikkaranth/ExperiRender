// SceneDesc.cpp
#include "Scene/SceneDesc.h"
#include <fstream>
#include <stdexcept>

std::vector<SceneDesc::SceneInfo> SceneDesc::getAllScenes(const std::string &jsonFilePath) {
    std::ifstream file(jsonFilePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + jsonFilePath);
    }

    nlohmann::json jsonData;
    try {
        file >> jsonData;
    } catch (const nlohmann::json::parse_error &e) {
        throw std::runtime_error("JSON parse error: " + std::string(e.what()));
    }

    std::vector<SceneInfo> scenes;

    // Check if the JSON contains a scenes array
    if (jsonData.contains("scenes") && jsonData["scenes"].is_array()) {
        for (const auto &sceneJson: jsonData["scenes"]) {
            // Validate each scene entry
            if (sceneJson.contains("scene_name") && sceneJson.contains("scene_filepath") &&
                sceneJson["scene_name"].is_string() && sceneJson["scene_filepath"].is_string()) {
                SceneInfo info;
                info.name = sceneJson["scene_name"];
                info.filePath = sceneJson["scene_filepath"];

                // Parse transformation data if present
                if (sceneJson.contains("transform")) {
                    info.hasTransform = true;

                    // Parse translation if present
                    if (sceneJson["transform"].contains("translate") &&
                        sceneJson["transform"]["translate"].is_array() &&
                        sceneJson["transform"]["translate"].size() == 3) {

                        info.translate.x = sceneJson["transform"]["translate"][0];
                        info.translate.y = sceneJson["transform"]["translate"][1];
                        info.translate.z = sceneJson["transform"]["translate"][2];
                    }

                    // Parse scale if present
                    if (sceneJson["transform"].contains("scale") && sceneJson["transform"]["scale"].is_array() &&
                        sceneJson["transform"]["scale"].size() == 3) {

                        info.scale.x = sceneJson["transform"]["scale"][0];
                        info.scale.y = sceneJson["transform"]["scale"][1];
                        info.scale.z = sceneJson["transform"]["scale"][2];
                    }

                    // Parse rotation if present
                    if (sceneJson["transform"].contains("rotate") && sceneJson["transform"]["rotate"].is_array() &&
                        sceneJson["transform"]["rotate"].size() == 3) {

                        info.rotate.x = sceneJson["transform"]["rotate"][0];
                        info.rotate.y = sceneJson["transform"]["rotate"][1];
                        info.rotate.z = sceneJson["transform"]["rotate"][2];
                    }
                }

                scenes.push_back(info);
            }
        }
    }
    // Backward compatibility for single scene format
    else if (jsonData.contains("scene_name") && jsonData.contains("scene_filepath")) {
        SceneInfo info;
        info.name = jsonData["scene_name"];
        info.filePath = jsonData["scene_filepath"];
        scenes.push_back(info);
    }

    if (scenes.empty()) {
        throw std::runtime_error("No valid scenes found in JSON file");
    }

    return scenes;
}

// SceneDesc.cpp
SceneDesc::HDRIInfo SceneDesc::getHDRIInfo(const std::string &jsonFilePath) {
    std::ifstream file(jsonFilePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + jsonFilePath);
    }

    nlohmann::json jsonData;
    try {
        file >> jsonData;
    } catch (const nlohmann::json::parse_error &e) {
        throw std::runtime_error("JSON parse error: " + std::string(e.what()));
    }

    HDRIInfo info;

    // Check if HDRI info exists in the JSON
    if (jsonData.contains("hdri") && jsonData["hdri"].is_object() && jsonData["hdri"].contains("filepath") &&
        jsonData["hdri"]["filepath"].is_string()) {

        info.filePath = jsonData["hdri"]["filepath"];
    } else {
        // Default HDRI path if not specified in JSON
        info.filePath = "../assets/HDRI/pretoria_gardens_4k.hdr";
    }

    return info;
}

// For backward compatibility
SceneDesc::SceneInfo SceneDesc::getSceneInfo(const std::string &jsonFilePath) {
    auto scenes = getAllScenes(jsonFilePath);
    return scenes[0]; // Return the first scene
}