#include "Primitives.hpp"
#include "Mesh.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <cmath>

namespace prim {

static constexpr float PI = 3.14159265358979323846f;

static void addTri(std::vector<VertexPN>& v, std::vector<unsigned int>& idx,
                   const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
                   const glm::vec3& n)
{
    unsigned int base = (unsigned int)v.size();
    v.push_back(VertexPN{a, n, glm::vec2(0.0f, 0.0f)});
    v.push_back(VertexPN{b, n, glm::vec2(0.0f, 0.0f)});
    v.push_back(VertexPN{c, n, glm::vec2(0.0f, 0.0f)});
    idx.push_back(base + 0);
    idx.push_back(base + 1);
    idx.push_back(base + 2);
}

Mesh makeDisc(float radius, int slices) {
    if (slices < 3) slices = 3;

    std::vector<VertexPN> v;
    std::vector<unsigned int> idx;

    v.reserve((size_t)slices + 2);
    idx.reserve((size_t)slices * 3);

    const glm::vec3 n(0, 1, 0);
    const glm::vec3 center(0, 0, 0);

    // Fan triangles: center - p(i) - p(i+1)
    for (int i = 0; i < slices; ++i) {
        float a0 = (float)i / (float)slices * 2.0f * PI;
        float a1 = (float)(i + 1) / (float)slices * 2.0f * PI;

        glm::vec3 p0(radius * std::cos(a0), 0.0f, radius * std::sin(a0));
        glm::vec3 p1(radius * std::cos(a1), 0.0f, radius * std::sin(a1));

        addTri(v, idx, center, p0, p1, n);
    }

    return Mesh::fromTriangles(v, idx);
}

} // namespace prim
