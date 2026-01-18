#include "Texture.hpp"
#include "Util.hpp"

#include <algorithm>

// MSYS2 stb header location:
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>


static GLuint uploadTexture(const unsigned char* pixels, int w, int h, int channels, bool mip) {
    if (!pixels || w <= 0 || h <= 0) return 0;

    GLenum fmt = (channels == 4) ? GL_RGBA : GL_RGB;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mip ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, (channels == 4) ? GL_RGBA8 : GL_RGB8, w, h, 0, fmt, GL_UNSIGNED_BYTE, pixels);
    if (mip) glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

Texture2D::~Texture2D() {
    if (m_id) glDeleteTextures(1, &m_id);
}

Texture2D::Texture2D(Texture2D&& other) noexcept {
    m_id = other.m_id;
    other.m_id = 0;
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this != &other) {
        if (m_id) glDeleteTextures(1, &m_id);
        m_id = other.m_id;
        other.m_id = 0;
    }
    return *this;
}

Texture2D Texture2D::fromFile(const std::string& path, bool mip) {
    Texture2D t;
    stbi_set_flip_vertically_on_load(1);

    int w = 0, h = 0, c = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 0);
    if (!data) {
        util::logWarn(std::string("Failed to load image: ") + path);
        return t;
    }

    t.m_id = uploadTexture(data, w, h, c, mip);
    stbi_image_free(data);

    if (t.m_id) util::logInfo(std::string("Loaded texture: ") + path);
    return t;
}

Texture2D Texture2D::fromMemory(const unsigned char* data, int sizeBytes, bool mip) {
    Texture2D t;
    stbi_set_flip_vertically_on_load(1);

    int w = 0, h = 0, c = 0;
    unsigned char* pixels = stbi_load_from_memory(data, sizeBytes, &w, &h, &c, 0);
    if (!pixels) {
        util::logWarn("Failed to decode embedded texture (stb_image).");
        return t;
    }

    t.m_id = uploadTexture(pixels, w, h, c, mip);
    stbi_image_free(pixels);
    return t;
}

Texture2D Texture2D::fromPixels(const unsigned char* pixels, int w, int h, int channels, bool mip) {
    Texture2D t;
    t.m_id = uploadTexture(pixels, w, h, channels, mip);
    return t;
}
