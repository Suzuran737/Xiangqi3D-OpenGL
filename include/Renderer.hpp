#pragma once

#include "Camera.hpp"
#include "Model.hpp"
#include "Primitives.hpp"
#include "Shader.hpp"
#include "XiangqiGame.hpp"

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>

class Renderer {
public:
    bool init(int viewportW, int viewportH);
    void resize(int viewportW, int viewportH);

    void draw(const OrbitCamera& cam, const XiangqiGame& game);

private:
    int m_w = 1;
    int m_h = 1;

    Shader m_basicShader;
    Shader m_lineShader;

    Model m_boardModel;
    bool m_hasBoardModel = false;
    glm::mat4 m_boardModelXform = glm::mat4(1.0f);

    std::unordered_map<std::string, Model> m_pieceModels;

    Mesh m_fallbackDisc;

    // Grid lines (we will draw even when using a board model)
    GLuint m_lineVAO = 0;
    GLuint m_lineVBO = 0;
    GLsizei m_lineVertexCount = 0;

    glm::vec3 boardToWorld(const Pos& p) const;
    std::string findBoardModelPath() const;
    std::string findPieceModelPath(const std::string& key) const;

    const Model* getPieceModelOrNull(const std::string& key);
    void ensureLineGrid();

    void computeBoardModelTransform();
};
