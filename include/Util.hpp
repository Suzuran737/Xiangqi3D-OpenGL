#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace util {

inline std::string readTextFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

inline bool fileExists(const std::string& path) {
    std::ifstream f(path.c_str());
    return (bool)f;
}

inline void logInfo(const std::string& msg) {
    std::cout << "[INFO] " << msg << std::endl;
}

inline void logWarn(const std::string& msg) {
    std::cout << "[WARN] " << msg << std::endl;
}

inline void logError(const std::string& msg) {
    std::cerr << "[ERROR] " << msg << std::endl;
}

// Minimal UTF-8 to Unicode codepoints (UTF-32) decoder.
inline std::vector<char32_t> utf8ToCodepoints(std::string_view s) {
    std::vector<char32_t> out;
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) {
            out.push_back(static_cast<char32_t>(c));
            i += 1;
        } else if ((c >> 5) == 0x6 && i + 1 < s.size()) {
            char32_t cp = ((c & 0x1F) << 6) | (static_cast<unsigned char>(s[i + 1]) & 0x3F);
            out.push_back(cp);
            i += 2;
        } else if ((c >> 4) == 0xE && i + 2 < s.size()) {
            char32_t cp = ((c & 0x0F) << 12) |
                          ((static_cast<unsigned char>(s[i + 1]) & 0x3F) << 6) |
                          (static_cast<unsigned char>(s[i + 2]) & 0x3F);
            out.push_back(cp);
            i += 3;
        } else if ((c >> 3) == 0x1E && i + 3 < s.size()) {
            char32_t cp = ((c & 0x07) << 18) |
                          ((static_cast<unsigned char>(s[i + 1]) & 0x3F) << 12) |
                          ((static_cast<unsigned char>(s[i + 2]) & 0x3F) << 6) |
                          (static_cast<unsigned char>(s[i + 3]) & 0x3F);
            out.push_back(cp);
            i += 4;
        } else {
            // Invalid byte sequence, skip.
            i += 1;
        }
    }
    return out;
}

} // namespace util
