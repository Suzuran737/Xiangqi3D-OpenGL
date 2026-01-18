#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <vector>

struct VertexPN {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    static Mesh fromTriangles(const std::vector<VertexPN>& verts, const std::vector<unsigned int>& indices);
    static Mesh fromTrianglesNonIndexed(const std::vector<VertexPN>& verts);

    void draw() const;

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    GLsizei m_elemCount = 0;
    bool m_indexed = false;
};
