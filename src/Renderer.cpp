#include "Renderer.hpp"

#include "Config.hpp"
#include "Util.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <vector>

static glm::vec3 sideColor(Side s) {
    return (s == Side::Red) ? glm::vec3(0.78f, 0.18f, 0.18f) : glm::vec3(0.12f, 0.12f, 0.12f);
}

static AABB transformAABB(const AABB& a, const glm::mat4& M) {
    glm::vec3 corners[8] = {
        {a.min.x, a.min.y, a.min.z},
        {a.max.x, a.min.y, a.min.z},
        {a.min.x, a.max.y, a.min.z},
        {a.max.x, a.max.y, a.min.z},
        {a.min.x, a.min.y, a.max.z},
        {a.max.x, a.min.y, a.max.z},
        {a.min.x, a.max.y, a.max.z},
        {a.max.x, a.max.y, a.max.z},
    };

    AABB out;
    out.min = glm::vec3(1e30f);
    out.max = glm::vec3(-1e30f);
    for (auto& c : corners) {
        glm::vec3 t = glm::vec3(M * glm::vec4(c, 1.0f));
        out.min = glm::min(out.min, t);
        out.max = glm::max(out.max, t);
    }
    return out;
}

bool Renderer::init(int viewportW, int viewportH) {
    m_w = viewportW;
    m_h = viewportH;

    try {
        m_basicShader = Shader("assets/shaders/basic.vert", "assets/shaders/basic.frag");
        m_lineShader  = Shader("assets/shaders/line.vert",  "assets/shaders/line.frag");
    } catch (const std::exception& e) {
        util::logError(e.what());
        return false;
    }

    // Highlight disc
    m_fallbackDisc = prim::makeDisc(0.23f, 32);

    // Always build grid (useful even with board model)
    ensureLineGrid();

    // Try load board model
    std::string boardPath = findBoardModelPath();
    if (!boardPath.empty()) {
        m_boardModel = Model(boardPath);
        m_hasBoardModel = m_boardModel.valid();
        if (m_hasBoardModel) {
            computeBoardModelTransform();
        }
    }
    if (!m_hasBoardModel) {
        util::logError("Board model not found or failed to load.");
        return false;
    }

    return true;
}

void Renderer::resize(int viewportW, int viewportH) {
    m_w = viewportW;
    m_h = viewportH;
    glViewport(0, 0, viewportW, viewportH);
}

glm::vec3 Renderer::boardToWorld(const Pos& p) const {
    float x = (p.x - 4) * cfg::CELL + cfg::BOARD_GRID_OFFSET_X;
    float z = (p.y - 4.5f) * cfg::CELL + cfg::BOARD_GRID_OFFSET_Z;
    return glm::vec3(x, cfg::BOARD_PLANE_Y + cfg::PIECE_Y + cfg::PIECE_Y_OFFSET, z);
}

std::string Renderer::findBoardModelPath() const {
    if (util::fileExists(cfg::BOARD_MODEL_GLB))  return cfg::BOARD_MODEL_GLB;
    if (util::fileExists(cfg::BOARD_MODEL_GLTF)) return cfg::BOARD_MODEL_GLTF;
    if (util::fileExists(cfg::BOARD_MODEL_OBJ))  return cfg::BOARD_MODEL_OBJ;
    return {};
}

std::string Renderer::findPieceModelPath(const std::string& key) const {
    const std::string base = cfg::PIECES_DIR + "/" + key;
    const std::string glb  = base + ".glb";
    const std::string gltf = base + ".gltf";
    const std::string obj  = base + ".obj";
    if (util::fileExists(glb))  return glb;
    if (util::fileExists(gltf)) return gltf;
    if (util::fileExists(obj))  return obj;
    return {};
}

const Model* Renderer::getPieceModelOrNull(const std::string& key) {
    auto it = m_pieceModels.find(key);
    if (it != m_pieceModels.end()) {
        return it->second.valid() ? &it->second : nullptr;
    }

    std::string path = findPieceModelPath(key);
    if (path.empty()) {
        m_pieceModels.emplace(key, Model());
        return nullptr;
    }

    Model m(path);
    bool ok = m.valid();
    m_pieceModels.emplace(key, std::move(m));
    return ok ? &m_pieceModels.at(key) : nullptr;
}

void Renderer::ensureLineGrid() {
    if (m_lineVAO) return;

    std::vector<glm::vec3> lines;
    lines.reserve((10 + 9) * 2);
    const float ox = cfg::BOARD_GRID_OFFSET_X;
    const float oz = cfg::BOARD_GRID_OFFSET_Z;

    for (int y = 0; y < 10; ++y) {
        float z = (y - 4.5f) * cfg::CELL + oz;
        glm::vec3 a(-4.0f * cfg::CELL + ox, cfg::BOARD_PLANE_Y + 0.001f, z);
        glm::vec3 b( 4.0f * cfg::CELL + ox, cfg::BOARD_PLANE_Y + 0.001f, z);
        lines.push_back(a); lines.push_back(b);
    }
    for (int x = 0; x < 9; ++x) {
        float xx = (x - 4) * cfg::CELL + ox;
        glm::vec3 a(xx, cfg::BOARD_PLANE_Y + 0.001f, -4.5f * cfg::CELL + oz);
        glm::vec3 b(xx, cfg::BOARD_PLANE_Y + 0.001f,  4.5f * cfg::CELL + oz);
        lines.push_back(a); lines.push_back(b);
    }

    m_lineVertexCount = static_cast<GLsizei>(lines.size());

    glGenVertexArrays(1, &m_lineVAO);
    glGenBuffers(1, &m_lineVBO);

    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(lines.size() * sizeof(glm::vec3)), lines.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    glBindVertexArray(0);
}

void Renderer::computeBoardModelTransform() {
    // Goal:
    // - Rotate so the thinnest axis becomes Y (up)
    // - Scale X/Z footprint to match the board size
    // - Translate to center at origin and align the top surface to BOARD_PLANE_Y
    const AABB a0 = m_boardModel.aabb();
    glm::vec3 size0 = a0.max - a0.min;

    // Find thinnest axis => likely thickness
    int thinAxis = 0;
    float thin = size0.x;
    if (size0.y < thin) { thin = size0.y; thinAxis = 1; }
    if (size0.z < thin) { thin = size0.z; thinAxis = 2; }

    glm::mat4 R(1.0f);
    if (thinAxis == 2) {
        // Z is thin => rotate Z->Y
        R = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1,0,0));
    } else if (thinAxis == 0) {
        // X is thin => rotate X->Y
        R = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0,0,1));
    }

    AABB ar = transformAABB(a0, R);
    glm::vec3 sizer = ar.max - ar.min;

    float targetX = cfg::BOARD_MODEL_WIDTH;
    float targetZ = cfg::BOARD_MODEL_DEPTH;
    float s = 1.0f;
    if (sizer.x > 1e-5f && sizer.z > 1e-5f) {
        s = std::min(targetX / sizer.x, targetZ / sizer.z);
    }

    glm::vec3 center = 0.5f * (ar.min + ar.max);
    float topY = ar.max.y;

    glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(s));
    glm::mat4 T = glm::translate(glm::mat4(1.0f),
        glm::vec3(-center.x * s, cfg::BOARD_PLANE_Y - topY * s, -center.z * s));

    m_boardModelXform = T * S * R;

    util::logInfo("Board model auto-fit applied (rotation+scale+top-align).");
}

void Renderer::draw(const OrbitCamera& cam, const XiangqiGame& game) {
    glViewport(0, 0, m_w, m_h);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);      // Disable culling to avoid negative-scale issues.
    glFrontFace(GL_CCW);          // Keep default winding.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 V = cam.view();
    glm::mat4 P = cam.projection((float)m_w / (float)m_h);

    // ----- Draw board -----
    m_basicShader.use();
    m_basicShader.setMat4("view", V);
    m_basicShader.setMat4("projection", P);
    m_basicShader.setVec3("lightDir", glm::normalize(glm::vec3(-1.0f, -1.5f, -0.8f)));

    m_basicShader.setInt("albedoMap", 0);

    if (cfg::BOARD_USE_ALBEDO && m_boardModel.hasAlbedo()) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_boardModel.albedoId());
        m_basicShader.setInt("useTexture", 1);
        m_basicShader.setVec3("baseColor", glm::vec3(1.0f));
    } else {
        m_basicShader.setInt("useTexture", 0);
        m_basicShader.setVec3("baseColor", glm::vec3(0.62f, 0.47f, 0.28f));
    }

    m_basicShader.setFloat("alpha", 1.0f);
    m_basicShader.setMat4("model", m_boardModelXform);
    m_boardModel.draw();

    glBindTexture(GL_TEXTURE_2D, 0);

    if (cfg::BOARD_DRAW_GRID) {
        m_lineShader.use();
        m_lineShader.setMat4("view", V);
        m_lineShader.setMat4("projection", P);
        m_lineShader.setMat4("model", glm::mat4(1.0f));
        m_lineShader.setVec3("color", glm::vec3(0.15f, 0.10f, 0.06f));
        glBindVertexArray(m_lineVAO);
        glDrawArrays(GL_LINES, 0, m_lineVertexCount);
        glBindVertexArray(0);
    }

    // ----- Highlights -----
    m_basicShader.use();
    m_basicShader.setMat4("view", V);
    m_basicShader.setMat4("projection", P);
    m_basicShader.setVec3("lightDir", glm::normalize(glm::vec3(-1.0f, -1.5f, -0.8f)));
    m_basicShader.setInt("useTexture", 0);

    if (game.selected()) {
        glm::vec3 wpos = boardToWorld(*game.selected());
        glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(wpos.x, cfg::BOARD_PLANE_Y + 0.01f, wpos.z));
        M = glm::scale(M, glm::vec3(1.5f));
        m_basicShader.setMat4("model", M);
        m_basicShader.setVec3("baseColor", glm::vec3(0.2f, 0.8f, 0.2f));
        m_basicShader.setFloat("alpha", 0.35f);
        m_fallbackDisc.draw();

        for (const auto& t : game.legalTargets()) {
            glm::vec3 wp = boardToWorld(t);
            glm::mat4 MM = glm::translate(glm::mat4(1.0f), glm::vec3(wp.x, cfg::BOARD_PLANE_Y + 0.01f, wp.z));
            m_basicShader.setMat4("model", MM);
            m_basicShader.setVec3("baseColor", glm::vec3(0.2f, 0.5f, 0.9f));
            m_basicShader.setFloat("alpha", 0.25f);
            m_fallbackDisc.draw();
        }
    }

    // ----- Pieces -----
    m_basicShader.setInt("albedoMap", 0);
    const auto& b = game.board();
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 9; ++x) {
            if (!b.cells[y][x]) continue;
            Piece p = *b.cells[y][x];
            Pos pos{x, y};

            glm::vec3 wpos = boardToWorld(pos);

            float scale = 1.0f;
            if (game.selected() && *game.selected() == pos) scale = 1.12f;

            m_basicShader.use();
            m_basicShader.setMat4("view", V);
            m_basicShader.setMat4("projection", P);
            m_basicShader.setVec3("lightDir", glm::normalize(glm::vec3(-1.0f, -1.5f, -0.8f)));
            m_basicShader.setFloat("alpha", 1.0f);

            const Model* model = getPieceModelOrNull(pieceKey(p));
            if (!model) {
                continue;
            }

            if (model->hasAlbedo()) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, model->albedoId());
                m_basicShader.setInt("useTexture", 1);
                m_basicShader.setVec3("baseColor", glm::vec3(1.0f));
            } else {
                m_basicShader.setInt("useTexture", 0);
                m_basicShader.setVec3("baseColor", sideColor(p.side));
            }

            glm::mat4 M = glm::translate(glm::mat4(1.0f), wpos);
            M = glm::scale(M, glm::vec3(cfg::PIECE_MODEL_SCALE * scale));
            M = M * model->suggestedTransform();
            m_basicShader.setMat4("model", M);
            model->draw();
            if (model->hasAlbedo()) {
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }
    }

    // ----- Captures animation -----
    for (const auto& c : game.captures()) {
        float k = (c.duration > 0.0f) ? std::min(1.0f, c.t / c.duration) : 1.0f;
        float alpha = 1.0f - k;
        float sc = 1.0f - 0.7f * k;
        glm::vec3 wpos = boardToWorld(c.pos);
        wpos.y -= 0.15f * k;

        m_basicShader.use();
        m_basicShader.setMat4("view", V);
        m_basicShader.setMat4("projection", P);
        m_basicShader.setVec3("lightDir", glm::normalize(glm::vec3(-1.0f, -1.5f, -0.8f)));
        m_basicShader.setFloat("alpha", alpha);

        const Model* model = getPieceModelOrNull(pieceKey(c.piece));
        if (!model) {
            continue;
        }

        if (model->hasAlbedo()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, model->albedoId());
            m_basicShader.setInt("useTexture", 1);
            m_basicShader.setVec3("baseColor", glm::vec3(1.0f));
        } else {
            m_basicShader.setInt("useTexture", 0);
            m_basicShader.setVec3("baseColor", sideColor(c.piece.side));
        }

        glm::mat4 M = glm::translate(glm::mat4(1.0f), wpos);
        M = glm::scale(M, glm::vec3(cfg::PIECE_MODEL_SCALE * sc));
        M = M * model->suggestedTransform();
        m_basicShader.setMat4("model", M);
        model->draw();
        if (model->hasAlbedo()) {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

}
