#pragma once

#include <glm/vec4.hpp>
#include <glm/vec3.hpp>

struct Vertex {

    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    alignas(16) glm::vec4 color;
    alignas(16) glm::vec3 tangent;
    alignas(16) glm::vec3 bitangent;
};