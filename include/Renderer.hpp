#pragma once

#include "Camera.hpp"
#include "Model.hpp"
#include "Primitives.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "TextRenderer.hpp"
#include "XiangqiGame.hpp"

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>
#include <vector>

struct UiRect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

struct MenuLayout {
    UiRect start;
    UiRect exit;
};

class Renderer {
public:
    bool init(int viewportW, int viewportH);
    void resize(int viewportW, int viewportH);

    void draw(const OrbitCamera& cam, const XiangqiGame& game);
    void drawMenu(const MenuLayout& layout, bool hoverStart, bool hoverExit, bool startEnabled);
    void beginPreload();
    bool preloadStep(int maxItems = 1);
    bool isPreloadReady() const;

private:
    int m_w = 1;
    int m_h = 1;

    Shader m_basicShader;
    Shader m_lineShader;

    TextRenderer m_text;
    Texture2D m_menuBg;

    Model m_boardModel;
    bool m_hasBoardModel = false;
    glm::mat4 m_boardModelXform = glm::mat4(1.0f);

    std::unordered_map<std::string, Model> m_pieceModels;

    Mesh m_fallbackDisc;
    Mesh m_uiQuad;

    // Grid lines (we will draw even when using a board model)
    GLuint m_lineVAO = 0;
    GLuint m_lineVBO = 0;
    GLsizei m_lineVertexCount = 0;

    std::string m_boardPath;
    bool m_boardLoaded = false;
    bool m_preloadFailed = false;
    std::vector<Piece> m_pendingPieces;
    size_t m_pendingIndex = 0;

    glm::vec3 boardToWorld(const Pos& p) const;
    std::string findBoardModelPath() const;
    std::string findPieceModelPath(const std::string& key) const;

    const Model* getPieceModelOrNull(const std::string& key);
    void ensureLineGrid();

    void computeBoardModelTransform();
};
