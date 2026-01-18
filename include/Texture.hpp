#pragma once
#include <glad/gl.h>
#include <string>
#include <vector>

class Texture2D {
public:
    Texture2D() = default;
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    bool valid() const { return m_id != 0; }
    GLuint id() const { return m_id; }

    // Load from file path (png/jpg/...)
    static Texture2D fromFile(const std::string& path, bool generateMipmaps = true);

    // Load from memory blob (compressed image bytes, e.g. embedded PNG)
    static Texture2D fromMemory(const unsigned char* data, int sizeBytes, bool generateMipmaps = true);

    // Load from raw RGBA/RGB pixels
    static Texture2D fromPixels(const unsigned char* pixels, int w, int h, int channels, bool generateMipmaps = true);

private:
    GLuint m_id = 0;
};
