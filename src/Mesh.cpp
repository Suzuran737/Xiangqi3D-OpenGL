#include "Mesh.hpp"

Mesh::~Mesh() {
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    m_ebo = m_vbo = m_vao = 0;
    m_elemCount = 0;
}

Mesh::Mesh(Mesh&& other) noexcept {
    m_vao = other.m_vao;
    m_vbo = other.m_vbo;
    m_ebo = other.m_ebo;
    m_elemCount = other.m_elemCount;
    m_indexed = other.m_indexed;
    other.m_vao = other.m_vbo = other.m_ebo = 0;
    other.m_elemCount = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        this->~Mesh();
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_ebo = other.m_ebo;
        m_elemCount = other.m_elemCount;
        m_indexed = other.m_indexed;
        other.m_vao = other.m_vbo = other.m_ebo = 0;
        other.m_elemCount = 0;
    }
    return *this;
}

Mesh Mesh::fromTriangles(const std::vector<VertexPN>& verts, const std::vector<unsigned int>& indices) {
    Mesh m;
    m.m_indexed = true;
    m.m_elemCount = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &m.m_vao);
    glGenBuffers(1, &m.m_vbo);
    glGenBuffers(1, &m.m_ebo);

    glBindVertexArray(m.m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m.m_vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(verts.size() * sizeof(VertexPN)), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPN), (void*)offsetof(VertexPN, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPN), (void*)offsetof(VertexPN, normal));

    glEnableVertexAttribArray(2); 
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPN), (void*)offsetof(VertexPN, uv));

    glBindVertexArray(0);
    return m;
}

Mesh Mesh::fromTrianglesNonIndexed(const std::vector<VertexPN>& verts) {
    Mesh m;
    m.m_indexed = false;
    m.m_elemCount = static_cast<GLsizei>(verts.size());

    glGenVertexArrays(1, &m.m_vao);
    glGenBuffers(1, &m.m_vbo);

    glBindVertexArray(m.m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m.m_vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(verts.size() * sizeof(VertexPN)), verts.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPN), (void*)offsetof(VertexPN, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPN), (void*)offsetof(VertexPN, normal));

    glEnableVertexAttribArray(2); // uv
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPN), (void*)offsetof(VertexPN, uv));

    glBindVertexArray(0);
    return m;
}

void Mesh::draw() const {
    glBindVertexArray(m_vao);
    if (m_indexed) {
        glDrawElements(GL_TRIANGLES, m_elemCount, GL_UNSIGNED_INT, nullptr);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, m_elemCount);
    }
    glBindVertexArray(0);
}
