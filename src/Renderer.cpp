#include "Renderer.hpp"

#include "Config.hpp"
#include "Util.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
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

static Mesh makeUiQuad() {
    std::vector<VertexPN> v;
    v.reserve(6);

    const glm::vec3 n(0.0f, 0.0f, 1.0f);
    const glm::vec3 p0(0.0f, 0.0f, 0.0f);
    const glm::vec3 p1(1.0f, 0.0f, 0.0f);
    const glm::vec3 p2(1.0f, 1.0f, 0.0f);
    const glm::vec3 p3(0.0f, 1.0f, 0.0f);

    v.push_back(VertexPN{p0, n, glm::vec2(0.0f, 0.0f)});
    v.push_back(VertexPN{p1, n, glm::vec2(1.0f, 0.0f)});
    v.push_back(VertexPN{p2, n, glm::vec2(1.0f, 1.0f)});

    v.push_back(VertexPN{p0, n, glm::vec2(0.0f, 0.0f)});
    v.push_back(VertexPN{p2, n, glm::vec2(1.0f, 1.0f)});
    v.push_back(VertexPN{p3, n, glm::vec2(0.0f, 1.0f)});

    return Mesh::fromTrianglesNonIndexed(v);
}

static float sine01(float t) {
    return 0.5f + 0.5f * std::sin(t);
}

static glm::mat4 makeShadowMatrix(const glm::vec3& lightDir, float planeY) {
    glm::vec4 P(0.0f, 1.0f, 0.0f, -planeY);
    glm::vec4 L(lightDir, 0.0f);
    float dot = glm::dot(P, L);

    glm::mat4 M(0.0f);
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            float v = dot * ((row == col) ? 1.0f : 0.0f) - L[row] * P[col];
            M[col][row] = v;
        }
    }

    return M;
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

    m_fallbackDisc = prim::makeDisc(0.23f, 32);
    m_uiQuad = makeUiQuad();

    ensureLineGrid();

    m_boardPath = findBoardModelPath();
    if (m_boardPath.empty()) {
        util::logError("Board model not found.");
        return false;
    }

    if (util::fileExists(cfg::MENU_BG_TEXTURE)) {
        m_menuBg = Texture2D::fromFile(cfg::MENU_BG_TEXTURE, false);
    } else {
        util::logWarn(std::string("Menu background not found: ") + cfg::MENU_BG_TEXTURE);
    }

    bool textOk = m_text.init(cfg::FONT_PATH, viewportW, viewportH);
    if (!textOk) {
        const char* fallbacks[] = {
            "C:/Windows/Fonts/msyh.ttc",
            "C:/Windows/Fonts/msyh.ttf",
            "C:/Windows/Fonts/simhei.ttf",
            "C:/Windows/Fonts/simsun.ttc",
        };
        for (const char* path : fallbacks) {
            if (util::fileExists(path) && m_text.init(path, viewportW, viewportH)) {
                util::logInfo(std::string("Using fallback font: ") + path);
                textOk = true;
                break;
            }
        }
    }
    if (!textOk) {
        util::logWarn("TextRenderer init failed (no font found).");
    }

    return true;
}

void Renderer::resize(int viewportW, int viewportH) {
    m_w = viewportW;
    m_h = viewportH;
    m_text.resize(viewportW, viewportH);
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
        lines.push_back(a);
        lines.push_back(b);
    }
    for (int x = 0; x < 9; ++x) {
        float xx = (x - 4) * cfg::CELL + ox;
        glm::vec3 a(xx, cfg::BOARD_PLANE_Y + 0.001f, -4.5f * cfg::CELL + oz);
        glm::vec3 b(xx, cfg::BOARD_PLANE_Y + 0.001f,  4.5f * cfg::CELL + oz);
        lines.push_back(a);
        lines.push_back(b);
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

void Renderer::beginPreload() {
    m_preloadFailed = false;
    m_boardLoaded = false;
    m_hasBoardModel = false;
    m_pendingPieces.clear();
    m_pendingIndex = 0;

    static const PieceType types[] = {
        PieceType::King,
        PieceType::Advisor,
        PieceType::Elephant,
        PieceType::Horse,
        PieceType::Rook,
        PieceType::Cannon,
        PieceType::Pawn,
    };

    Side sides[] = {Side::Red, Side::Black};
    for (Side s : sides) {
        for (PieceType t : types) {
            m_pendingPieces.push_back(Piece{s, t});
        }
    }
}

bool Renderer::preloadStep(int maxItems) {
    int loaded = 0;
    while (loaded < maxItems) {
        if (m_preloadFailed) break;

        if (!m_boardLoaded) {
            m_boardModel = Model(m_boardPath);
            m_hasBoardModel = m_boardModel.valid();
            if (!m_hasBoardModel) {
                util::logError("Board model failed to load.");
                m_preloadFailed = true;
                break;
            }
            computeBoardModelTransform();
            m_boardLoaded = true;
            loaded++;
            continue;
        }

        if (m_pendingIndex < m_pendingPieces.size()) {
            Piece p = m_pendingPieces[m_pendingIndex++];
            (void)getPieceModelOrNull(pieceKey(p));
            loaded++;
            continue;
        }

        break;
    }

    return isPreloadReady();
}

bool Renderer::isPreloadReady() const {
    return !m_preloadFailed && m_boardLoaded && m_pendingIndex >= m_pendingPieces.size();
}

void Renderer::computeBoardModelTransform() {
    const AABB a0 = m_boardModel.aabb();
    glm::vec3 size0 = a0.max - a0.min;

    int thinAxis = 0;
    float thin = size0.x;
    if (size0.y < thin) { thin = size0.y; thinAxis = 1; }
    if (size0.z < thin) { thin = size0.z; thinAxis = 2; }

    glm::mat4 R(1.0f);
    if (thinAxis == 2) {
        R = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1,0,0));
    } else if (thinAxis == 0) {
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
    if (!m_boardLoaded) {
        glViewport(0, 0, m_w, m_h);
        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return;
    }

    glViewport(0, 0, m_w, m_h);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 V = cam.view();
    glm::mat4 P = cam.projection((float)m_w / (float)m_h);
    glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.5f, -0.8f));

    m_basicShader.use();
    m_basicShader.setMat4("view", V);
    m_basicShader.setMat4("projection", P);
    m_basicShader.setVec3("lightDir", lightDir);
    m_basicShader.setVec3("viewPos", cam.position());
    m_basicShader.setFloat("specularStrength", 0.22f);
    m_basicShader.setFloat("shininess", 32.0f);

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

    m_basicShader.use();
    m_basicShader.setMat4("view", V);
    m_basicShader.setMat4("projection", P);
    m_basicShader.setVec3("lightDir", lightDir);
    m_basicShader.setVec3("viewPos", cam.position());
    m_basicShader.setFloat("specularStrength", 0.0f);
    m_basicShader.setFloat("shininess", 8.0f);
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

    const auto& b = game.board();

    // Draw planar shadows for pieces onto the board.
    glm::mat4 shadowProj = makeShadowMatrix(lightDir, cfg::BOARD_PLANE_Y + 0.002f);
    glDepthMask(GL_FALSE);
    m_basicShader.use();
    m_basicShader.setMat4("view", V);
    m_basicShader.setMat4("projection", P);
    m_basicShader.setVec3("lightDir", lightDir);
    m_basicShader.setVec3("viewPos", cam.position());
    m_basicShader.setInt("useTexture", 0);
    m_basicShader.setVec3("baseColor", glm::vec3(0.0f, 0.0f, 0.0f));
    m_basicShader.setFloat("alpha", 0.28f);
    m_basicShader.setFloat("specularStrength", 0.0f);
    m_basicShader.setFloat("shininess", 8.0f);

    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 9; ++x) {
            if (!b.cells[y][x]) continue;
            Piece p = *b.cells[y][x];
            Pos pos{x, y};

            glm::vec3 wpos = boardToWorld(pos);
            float scale = 1.0f;
            if (game.selected() && *game.selected() == pos) scale = 1.12f;

            const Model* model = getPieceModelOrNull(pieceKey(p));
            if (!model) continue;

            glm::mat4 M = glm::translate(glm::mat4(1.0f), wpos);
            M = glm::scale(M, glm::vec3(cfg::PIECE_MODEL_SCALE * scale));
            M = M * model->suggestedTransform();

            m_basicShader.setMat4("model", shadowProj * M);
            model->draw();
        }
    }
    glDepthMask(GL_TRUE);

    m_basicShader.setFloat("specularStrength", 0.26f);
    m_basicShader.setFloat("shininess", 32.0f);
    m_basicShader.setInt("albedoMap", 0);
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
            m_basicShader.setVec3("lightDir", lightDir);
            m_basicShader.setVec3("viewPos", cam.position());
            m_basicShader.setFloat("specularStrength", 0.26f);
            m_basicShader.setFloat("shininess", 32.0f);
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

    for (const auto& c : game.captures()) {
        float k = (c.duration > 0.0f) ? std::min(1.0f, c.t / c.duration) : 1.0f;
        float alpha = 1.0f - k;
        float sc = 1.0f - 0.7f * k;
        glm::vec3 wpos = boardToWorld(c.pos);
        wpos.y -= 0.15f * k;

        m_basicShader.use();
        m_basicShader.setMat4("view", V);
        m_basicShader.setMat4("projection", P);
        m_basicShader.setVec3("lightDir", lightDir);
        m_basicShader.setVec3("viewPos", cam.position());
        m_basicShader.setFloat("specularStrength", 0.26f);
        m_basicShader.setFloat("shininess", 32.0f);
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

void Renderer::drawMenu(const MenuLayout& layout, bool hoverStart, bool hoverExit, bool startEnabled) {
    glViewport(0, 0, m_w, m_h);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 V = glm::mat4(1.0f);
    glm::mat4 P = glm::ortho(0.0f, (float)m_w, 0.0f, (float)m_h);

    m_basicShader.use();
    m_basicShader.setMat4("view", V);
    m_basicShader.setMat4("projection", P);
    m_basicShader.setVec3("lightDir", glm::vec3(0.0f, 0.0f, -1.0f));
    m_basicShader.setVec3("viewPos", glm::vec3(0.0f, 0.0f, 1.0f));
    m_basicShader.setInt("useTexture", 0);
    m_basicShader.setFloat("alpha", 1.0f);
    m_basicShader.setFloat("specularStrength", 0.0f);
    m_basicShader.setFloat("shininess", 8.0f);
    m_basicShader.setInt("albedoMap", 0);

    auto drawRect = [&](const UiRect& r, const glm::vec3& color, float alpha) {
        glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(r.x, r.y, 0.0f));
        M = glm::scale(M, glm::vec3(r.w, r.h, 1.0f));
        m_basicShader.setMat4("model", M);
        m_basicShader.setVec3("baseColor", color);
        m_basicShader.setFloat("alpha", alpha);
        m_uiQuad.draw();
    };

    auto drawTexturedRect = [&](const UiRect& r, GLuint tex) {
        if (!tex) return;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        m_basicShader.setInt("useTexture", 1);
        m_basicShader.setVec3("baseColor", glm::vec3(1.0f));
        m_basicShader.setFloat("alpha", 1.0f);
        glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(r.x, r.y, 0.0f));
        M = glm::scale(M, glm::vec3(r.w, r.h, 1.0f));
        m_basicShader.setMat4("model", M);
        m_uiQuad.draw();
        glBindTexture(GL_TEXTURE_2D, 0);
        m_basicShader.setInt("useTexture", 0);
    };

    if (m_menuBg.valid()) {
        UiRect bg{0.0f, 0.0f, (float)m_w, (float)m_h};
        drawTexturedRect(bg, m_menuBg.id());
    }

    auto drawWoodButton = [&](const UiRect& r, bool hover, bool enabled) {
        glm::vec3 base = enabled ? glm::vec3(0.48f, 0.33f, 0.18f) : glm::vec3(0.30f, 0.25f, 0.20f);
        if (hover && enabled) base = glm::vec3(0.54f, 0.38f, 0.22f);
        glm::vec3 light = base + glm::vec3(0.06f, 0.05f, 0.04f);
        glm::vec3 dark = base - glm::vec3(0.12f, 0.09f, 0.07f);

        drawRect(r, base, 1.0f);

        // Subtle interior gradient for depth.
        UiRect topHalf{r.x, r.y + r.h * 0.5f, r.w, r.h * 0.5f};
        UiRect botHalf{r.x, r.y, r.w, r.h * 0.5f};
        drawRect(topHalf, light, 0.18f);
        drawRect(botHalf, dark, 0.22f);

        // Simple wood grain: thin, semi-transparent stripes with slight variation.
        const int stripes = 9;
        float stripeH = std::max(2.0f, r.h * 0.03f);
        float edge = 5.0f;
        for (int i = 0; i < stripes; ++i) {
            float t = (float)(i + 1) / (float)(stripes + 1);
            float y = r.y + r.h * t - stripeH * 0.5f;
            float v = (sine01(t * 6.28318f * 2.5f + 1.7f) - 0.5f) * 0.08f;
            glm::vec3 grain = base + glm::vec3(v);
            UiRect stripe{r.x + edge, y, r.w - edge * 2.0f, stripeH};
            drawRect(stripe, grain, 0.35f);
        }

        float inset = 6.0f;
        UiRect top{r.x + inset, r.y + r.h - inset, r.w - inset * 2.0f, inset};
        UiRect bottom{r.x + inset, r.y, r.w - inset * 2.0f, inset};
        UiRect left{r.x, r.y + inset, inset, r.h - inset * 2.0f};
        UiRect right{r.x + r.w - inset, r.y + inset, inset, r.h - inset * 2.0f};

        drawRect(top, light, 0.9f);
        drawRect(left, light, 0.6f);
        drawRect(bottom, dark, 0.85f);
        drawRect(right, dark, 0.85f);
    };

    drawWoodButton(layout.start, hoverStart, startEnabled);
    drawWoodButton(layout.exit, hoverExit, true);

    const char* startLabel = u8"\u5f00\u59cb";
    const char* exitLabel = u8"\u9000\u51fa";
    float textScale = 0.9f;

    TextMetrics startMetrics = m_text.measureText(startLabel, textScale);
    float startX = layout.start.x + (layout.start.w - startMetrics.width) * 0.5f;
    float startY = layout.start.y + layout.start.h * 0.5f + (startMetrics.descent - startMetrics.ascent) * 0.5f;
    m_text.renderText(startLabel, startX, startY, textScale,
        startEnabled ? glm::vec3(0.96f, 0.94f, 0.90f) : glm::vec3(0.70f, 0.68f, 0.64f));

    TextMetrics exitMetrics = m_text.measureText(exitLabel, textScale);
    float exitX = layout.exit.x + (layout.exit.w - exitMetrics.width) * 0.5f;
    float exitY = layout.exit.y + layout.exit.h * 0.5f + (exitMetrics.descent - exitMetrics.ascent) * 0.5f;
    m_text.renderText(exitLabel, exitX, exitY, textScale, glm::vec3(0.96f, 0.94f, 0.90f));
}
