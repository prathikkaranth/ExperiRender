#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <map>
#include <optional>
#include <cassert>

// Structure to hold model path and name information
struct ModelEntry {
    std::string name;  // Name to use in loadedScenes map
    std::string path;  // Path to the GLTF file
};

