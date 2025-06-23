#pragma once

#include <glm/glm.hpp>


class PointLight {
public:
    PointLight(glm::vec3 position, glm::vec3 color);

    const glm::vec3 &position() const { return m_position; }

    const glm::vec3 &color() const { return m_color; }

    void setPosition(const glm::vec3 &position) { m_position = position; }

private:
    glm::vec3 m_position;
    glm::vec3 m_color;
};