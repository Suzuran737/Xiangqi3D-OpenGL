#pragma once
#include <glad/gl.h>
#include <string>
#include <vector>

// 2D 纹理封装
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

    // 从文件路径加载（png/jpg/...）
    static Texture2D fromFile(const std::string& path, bool generateMipmaps = true);

    // 从内存数据加载（压缩图片字节，如嵌入式 PNG）
    static Texture2D fromMemory(const unsigned char* data, int sizeBytes, bool generateMipmaps = true);

    // 从原始 RGBA/RGB 像素加载
    static Texture2D fromPixels(const unsigned char* pixels, int w, int h, int channels, bool generateMipmaps = true);

private:
    GLuint m_id = 0;
};
