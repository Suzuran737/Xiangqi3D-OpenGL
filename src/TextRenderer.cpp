#include "TextRenderer.hpp"

#include "Util.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glm/gtc/matrix_transform.hpp>

// 释放字形纹理与渲染缓冲
TextRenderer::~TextRenderer() {
    for (auto& [cp, g] : m_glyphs) {
        if (g.texture) glDeleteTextures(1, &g.texture);
    }
    m_glyphs.clear();

    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    m_vbo = m_vao = 0;

    if (m_ftFace) {
        FT_Done_Face((FT_Face)m_ftFace);
        m_ftFace = nullptr;
    }
    if (m_ftLib) {
        FT_Done_FreeType((FT_Library)m_ftLib);
        m_ftLib = nullptr;
    }
}

// 初始化文字渲染器并创建基础缓冲
bool TextRenderer::init(const std::string& fontPath, int viewportW, int viewportH) {
    m_w = viewportW;
    m_h = viewportH;

    try {
        m_shader = Shader("assets/shaders/text.vert", "assets/shaders/text.frag");
    } catch (const std::exception& e) {
        util::logError(e.what());
        return false;
    }

    // 初始化字体库并加载字体文件
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        util::logWarn("Failed to init FreeType");
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        util::logWarn(std::string("Failed to load font: ") + fontPath);
        FT_Done_FreeType(ft);
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);

    m_ftLib = (void*)ft;
    m_ftFace = (void*)face;

    // 配置文字绘制用的顶点数据结构
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 预加载界面常用字符，减少首次绘制开销
    preload("0123456789()ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz:.- ");

    util::logInfo("TextRenderer ready");
    return true;
}

void TextRenderer::resize(int viewportW, int viewportH) {
    m_w = viewportW;
    m_h = viewportH;
}

// 加载单个字形并生成纹理
bool TextRenderer::loadGlyph(char32_t cp) {
    if (!m_ftFace) return false;
    FT_Face face = (FT_Face)m_ftFace;

    if (FT_Load_Char(face, (FT_ULong)cp, FT_LOAD_RENDER)) {
        return false;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        face->glyph->bitmap.width,
        face->glyph->bitmap.rows,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        face->glyph->bitmap.buffer
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    Glyph g;
    g.texture = tex;
    g.size = glm::ivec2((int)face->glyph->bitmap.width, (int)face->glyph->bitmap.rows);
    g.bearing = glm::ivec2((int)face->glyph->bitmap_left, (int)face->glyph->bitmap_top);
    g.advance = (unsigned int)face->glyph->advance.x;

    m_glyphs[cp] = g;
    return true;
}

// 预加载字符串所需字形
void TextRenderer::preload(const std::string& utf8) {
    auto cps = util::utf8ToCodepoints(utf8);
    // 遍历字符并预加载字形
    for (auto cp : cps) {
        if (m_glyphs.find(cp) == m_glyphs.end()) {
            loadGlyph(cp);
        }
    }
}

// 计算文字显示尺寸
TextMetrics TextRenderer::measureText(const std::string& utf8, float scale) {
    TextMetrics m;
    if (!m_ftFace) return m;

    preload(utf8);

    float width = 0.0f;
    float maxAscent = 0.0f;
    float maxDescent = 0.0f;

    auto cps = util::utf8ToCodepoints(utf8);
    for (auto cp : cps) {
        auto it = m_glyphs.find(cp);
        if (it == m_glyphs.end()) continue;
        const Glyph& ch = it->second;

        float ascent = (float)ch.bearing.y;
        float descent = (float)ch.size.y - (float)ch.bearing.y;
        if (ascent > maxAscent) maxAscent = ascent;
        if (descent > maxDescent) maxDescent = descent;

        width += (ch.advance / 64.0f);
    }

    m.width = width * scale;
    m.ascent = maxAscent * scale;
    m.descent = maxDescent * scale;
    return m;
}

// 逐字绘制文字到屏幕
void TextRenderer::renderText(const std::string& utf8, float x, float y, float scale, const glm::vec3& color) {
    if (!m_ftFace) return;

    preload(utf8);

    // 开启混合以支持透明文字
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shader.use();
    // 使用正交投影按像素绘制
    glm::mat4 proj = glm::ortho(0.0f, (float)m_w, 0.0f, (float)m_h);
    m_shader.setMat4("projection", proj);
    m_shader.setVec3("textColor", color);
    m_shader.setInt("text", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_vao);

    float xCursor = x;
    auto cps = util::utf8ToCodepoints(utf8);

    // 逐个字符生成四边形并绘制
    for (auto cp : cps) {
        auto it = m_glyphs.find(cp);
        if (it == m_glyphs.end()) continue;
        const Glyph& ch = it->second;

        float xpos = xCursor + (float)ch.bearing.x * scale;
        float ypos = y - (float)(ch.size.y - ch.bearing.y) * scale;

        float w = (float)ch.size.x * scale;
        float h = (float)ch.size.y * scale;

        // 当前字符的屏幕四边形顶点
        float vertices[6][4] = {
            {xpos,     ypos + h,   0.0f, 0.0f},
            {xpos,     ypos,       0.0f, 1.0f},
            {xpos + w, ypos,       1.0f, 1.0f},

            {xpos,     ypos + h,   0.0f, 0.0f},
            {xpos + w, ypos,       1.0f, 1.0f},
            {xpos + w, ypos + h,   1.0f, 0.0f},
        };

        glBindTexture(GL_TEXTURE_2D, ch.texture);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 更新游标位置
        xCursor += (ch.advance / 64.0f) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
