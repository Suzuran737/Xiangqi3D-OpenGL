#include "Model.hpp"

#include "Util.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/material.h>

#include <algorithm>
#include <cfloat>
#include <cstdlib>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

static glm::mat4 aiToGlm(const aiMatrix4x4& m) {
    // glm is column-major: mat[col][row]
    glm::mat4 r;
    r[0][0] = m.a1; r[1][0] = m.a2; r[2][0] = m.a3; r[3][0] = m.a4;
    r[0][1] = m.b1; r[1][1] = m.b2; r[2][1] = m.b3; r[3][1] = m.b4;
    r[0][2] = m.c1; r[1][2] = m.c2; r[2][2] = m.c3; r[3][2] = m.c4;
    r[0][3] = m.d1; r[1][3] = m.d2; r[2][3] = m.d3; r[3][3] = m.d4;
    return r;
}

static Mesh processMesh(aiMesh* mesh, const glm::mat4& xform, AABB& aabb) {
    std::vector<VertexPN> verts;
    verts.reserve(mesh->mNumVertices);

    glm::mat3 nmat = glm::transpose(glm::inverse(glm::mat3(xform)));

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        glm::vec3 p(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        glm::vec3 tp = glm::vec3(xform * glm::vec4(p, 1.0f));

        VertexPN v{};
        v.pos = tp;

        if (mesh->HasNormals()) {
            glm::vec3 n(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            v.normal = glm::normalize(nmat * n);
        } else {
            v.normal = glm::vec3(0, 1, 0);
        }

        if (mesh->HasTextureCoords(0)) {
            v.uv = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        } else {
            v.uv = glm::vec2(0.0f);
        }

        aabb.min = glm::min(aabb.min, tp);
        aabb.max = glm::max(aabb.max, tp);

        verts.push_back(v);
    }

    std::vector<unsigned int> indices;
    indices.reserve(mesh->mNumFaces * 3);
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& f = mesh->mFaces[i];
        for (unsigned int j = 0; j < f.mNumIndices; ++j) indices.push_back(f.mIndices[j]);
    }

    return Mesh::fromTriangles(verts, indices);
}

static void processNode(aiNode* node, const aiScene* scene, const glm::mat4& parent, std::vector<Mesh>& out, AABB& aabb) {
    glm::mat4 local = aiToGlm(node->mTransformation);
    glm::mat4 xform = parent * local;

    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* m = scene->mMeshes[node->mMeshes[i]];
        out.push_back(processMesh(m, xform, aabb));
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processNode(node->mChildren[i], scene, xform, out, aabb);
    }
}

static bool tryLoadAlbedoFromMaterial(Texture2D& out, const aiScene* scene, aiMaterial* mat, const std::string& modelPath) {
    aiString texPath;

    // glTF2 usually uses BASE_COLOR; it may also use DIFFUSE.
    bool found = (mat->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) == aiReturn_SUCCESS) ||
                 (mat->GetTexture(aiTextureType_DIFFUSE,   0, &texPath) == aiReturn_SUCCESS);

    if (!found) return false;

    std::string tpath = texPath.C_Str();
    if (tpath.empty()) return false;

    // Embedded texture in GLB: "*0", "*1", ...
    if (!tpath.empty() && tpath[0] == '*') {
        int idx = std::atoi(tpath.c_str() + 1);
        if (idx >= 0 && idx < (int)scene->mNumTextures) {
            const aiTexture* t = scene->mTextures[idx];
            if (!t) return false;

            if (t->mHeight == 0) {
                // Compressed bytes in pcData, length = mWidth
                out = Texture2D::fromMemory(
                    reinterpret_cast<const unsigned char*>(t->pcData),
                    (int)t->mWidth,
                    true
                );
                return out.valid();
            } else {
                // Raw texels stored as aiTexel (BGRA)
                std::vector<unsigned char> pixels;
                pixels.resize((size_t)t->mWidth * (size_t)t->mHeight * 4);

                for (unsigned int i = 0; i < t->mWidth * t->mHeight; ++i) {
                    const aiTexel& px = t->pcData[i];
                    pixels[i * 4 + 0] = px.r;
                    pixels[i * 4 + 1] = px.g;
                    pixels[i * 4 + 2] = px.b;
                    pixels[i * 4 + 3] = px.a;
                }

                out = Texture2D::fromPixels(pixels.data(), (int)t->mWidth, (int)t->mHeight, 4, true);
                return out.valid();
            }
        }
        return false;
    }

    // External texture path relative to model directory
    std::string modelDir = modelPath;
    auto slash = modelDir.find_last_of("/\\");
    if (slash != std::string::npos) modelDir = modelDir.substr(0, slash);

    std::string full = modelDir + "/" + tpath;
    out = Texture2D::fromFile(full, true);
    return out.valid();
}

Model::Model(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices
    );

    if (!scene || !scene->mRootNode) {
        util::logWarn(std::string("Assimp failed to load model: ") + path + " (" + importer.GetErrorString() + ")");
        return;
    }

    AABB aabb;
    aabb.min = glm::vec3( FLT_MAX,  FLT_MAX,  FLT_MAX);
    aabb.max = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    processNode(scene->mRootNode, scene, glm::mat4(1.0f), m_meshes, aabb);

    if (!m_meshes.empty()) {
        m_aabb = aabb;
    }

    // ---- Fast-path: load ONE albedo texture for the whole model ----
    // (For chessboard it's usually a single material anyway.)
    bool albedoLoaded = false;
    if (scene->mNumMeshes > 0 && scene->mNumMaterials > 0) {
        // Try materials in order of meshes until we find one with albedo/diffuse texture.
        for (unsigned int mi = 0; mi < scene->mNumMeshes && !albedoLoaded; ++mi) {
            int matIndex = (int)scene->mMeshes[mi]->mMaterialIndex;
            if (matIndex < 0 || matIndex >= (int)scene->mNumMaterials) continue;

            aiMaterial* mat = scene->mMaterials[matIndex];
            if (!mat) continue;

            if (tryLoadAlbedoFromMaterial(m_albedo, scene, mat, path)) {
                util::logInfo("Loaded board albedo texture from model material.");
                albedoLoaded = true;
                break;
            }
        }
    }

    if (!albedoLoaded) {
        util::logWarn("No albedo/diffuse texture found in model material (board will render with solid color).");
    }

    // ---- Build suggested transform: center XZ, bottom to y=0, scale to reasonable size ----
    if (!m_meshes.empty()) {
        const AABB a = m_aabb;
        glm::vec3 size = a.max - a.min;
        glm::vec3 center = 0.5f * (a.min + a.max);

        // Move center to origin in XZ, and bottom to y=0
        glm::mat4 T = glm::translate(glm::mat4(1.0f),
            glm::vec3(-center.x, -a.min.y, -center.z));

        // Scale so that the largest horizontal extent becomes 1.0 unit (roughly)
        float horiz = std::max(size.x, size.z);
        float s = 1.0f;
        if (horiz > 1e-5f) s = 1.0f / horiz;

        glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(s));

        m_suggested = S * T; // first translate then scale
    }

    util::logInfo(std::string("Loaded model: ") + path + " meshes=" + std::to_string(m_meshes.size()));
}

void Model::draw() const {
    for (const auto& m : m_meshes) {
        m.draw();
    }
}
