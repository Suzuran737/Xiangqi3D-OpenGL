#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

glm::vec3 OrbitCamera::position() const {
    float yaw = glm::radians(yawDeg);
    float pitch = glm::radians(pitchDeg);

    glm::vec3 dir;
    dir.x = std::cos(pitch) * std::cos(yaw);
    dir.y = std::sin(pitch);
    dir.z = std::cos(pitch) * std::sin(yaw);

    // Camera looks toward target from opposite direction
    return target + dir * distance;
}

glm::mat4 OrbitCamera::view() const {
    return glm::lookAt(position(), target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 OrbitCamera::projection(float aspect) const {
    return glm::perspective(glm::radians(40.0f), aspect, 0.1f, 200.0f);
}

Ray OrbitCamera::screenRay(double mouseX, double mouseY, int viewportW, int viewportH) const {
    float x = (2.0f * (float)mouseX) / (float)viewportW - 1.0f;
    float y = 1.0f - (2.0f * (float)mouseY) / (float)viewportH;

    glm::vec4 rayClip(x, y, -1.0f, 1.0f);
    glm::mat4 proj = projection((float)viewportW / (float)viewportH);
    glm::mat4 invProj = glm::inverse(proj);
    glm::vec4 rayEye = invProj * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    glm::mat4 invView = glm::inverse(view());
    glm::vec4 rayWorld4 = invView * rayEye;
    glm::vec3 dir = glm::normalize(glm::vec3(rayWorld4));

    Ray r;
    r.origin = position();
    r.dir = dir;
    return r;
}
