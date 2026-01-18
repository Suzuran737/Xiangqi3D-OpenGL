#pragma once

#include "Shader.hpp"

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <string>
#include <unordered_map>

struct Glyph {
    GLuint texture = 0;
    glm::ivec2 size{};
    glm::ivec2 bearing{};
    unsigned int advance = 0;
};

struct TextMetrics {
    float width = 0.0f;
    float ascent = 0.0f;
    float descent = 0.0f;
};

class TextRenderer {
public:
    TextRenderer() = default;
    ~TextRenderer();

    bool init(const std::string& fontPath, int viewportW, int viewportH);
    void resize(int viewportW, int viewportH);

    void preload(const std::string& utf8);
    TextMetrics measureText(const std::string& utf8, float scale);
    void renderText(const std::string& utf8, float x, float y, float scale, const glm::vec3& color);

private:
    bool loadGlyph(char32_t cp);

    Shader m_shader;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;

    int m_w = 1;
    int m_h = 1;

    void* m_ftLib = nullptr;
    void* m_ftFace = nullptr;

    std::unordered_map<char32_t, Glyph> m_glyphs;
};
