#pragma once

#include "Mesh.hpp"
#include "Texture.hpp"

#include <glm/glm.hpp>
#include <string>
#include <vector>

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

class Model {
public:
    Model() = default;
    explicit Model(const std::string& path);

    bool valid() const { return !m_meshes.empty(); }
    void draw() const;

    const AABB& aabb() const { return m_aabb; }

    // Accessors used by Renderer.
    bool hasAlbedo() const { return m_albedo.valid(); }
    GLuint albedoId() const { return m_albedo.id(); }
    const glm::mat4& suggestedTransform() const { return m_suggested; }

private:
    std::vector<Mesh> m_meshes;
    AABB m_aabb{{0,0,0},{0,0,0}};

    Texture2D m_albedo;
    glm::mat4 m_suggested{1.0f};
};