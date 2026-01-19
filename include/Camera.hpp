#pragma once

#include <glm/glm.hpp>

// 屏幕射线（用于拾取）
struct Ray {
    glm::vec3 origin;
    glm::vec3 dir;
};

// 轨道相机：绕目标点旋转
class OrbitCamera {
public:
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    float yawDeg = 45.0f;   // 绕 Y 轴
    float pitchDeg = 45.0f; // 上下俯仰
    float distance = 14.0f;

    glm::vec3 position() const;
    glm::mat4 view() const;
    glm::mat4 projection(float aspect) const;

    Ray screenRay(double mouseX, double mouseY, int viewportW, int viewportH) const;
};
