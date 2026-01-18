#pragma once

#include <glm/glm.hpp>

struct Ray {
    glm::vec3 origin;
    glm::vec3 dir;
};

class OrbitCamera {
public:
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    float yawDeg = 45.0f;   // around Y axis
    float pitchDeg = 45.0f; // up/down
    float distance = 14.0f;

    glm::vec3 position() const;
    glm::mat4 view() const;
    glm::mat4 projection(float aspect) const;

    Ray screenRay(double mouseX, double mouseY, int viewportW, int viewportH) const;
};
