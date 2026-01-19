#include "Shader.hpp"

#include "Util.hpp"

#include <stdexcept>

// 编译单个着色器
static GLuint compile(GLenum type, const std::string& src) {
    GLuint s = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(s, 1, &c, nullptr);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::string msg = (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment");
        msg += " shader compile failed: ";
        msg += log;
        glDeleteShader(s);
        throw std::runtime_error(msg);
    }
    return s;
}

// 读取源码，编译并链接程序
Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vs = util::readTextFile(vertexPath);
    std::string fs = util::readTextFile(fragmentPath);

    GLuint v = compile(GL_VERTEX_SHADER, vs);
    GLuint f = compile(GL_FRAGMENT_SHADER, fs);

    m_program = glCreateProgram();
    glAttachShader(m_program, v);
    glAttachShader(m_program, f);
    glLinkProgram(m_program);

    glDeleteShader(v);
    glDeleteShader(f);

    GLint ok = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(m_program, sizeof(log), nullptr, log);
        glDeleteProgram(m_program);
        m_program = 0;
        throw std::runtime_error(std::string("Shader program link failed: ") + log);
    }
}

// 释放着色器程序
Shader::~Shader() {
    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

Shader::Shader(Shader&& other) noexcept {
    m_program = other.m_program;
    other.m_program = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_program) glDeleteProgram(m_program);
        m_program = other.m_program;
        other.m_program = 0;
    }
    return *this;
}

// 使用当前着色器
void Shader::use() const {
    glUseProgram(m_program);
}

// 设置四乘四矩阵着色器变量
void Shader::setMat4(const std::string& name, const glm::mat4& m) const {
    GLint loc = glGetUniformLocation(m_program, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, &m[0][0]);
}

// 设置三维向量着色器变量
void Shader::setVec3(const std::string& name, const glm::vec3& v) const {
    GLint loc = glGetUniformLocation(m_program, name.c_str());
    glUniform3f(loc, v.x, v.y, v.z);
}

// 设置四维向量着色器变量
void Shader::setVec4(const std::string& name, const glm::vec4& v) const {
    GLint loc = glGetUniformLocation(m_program, name.c_str());
    glUniform4f(loc, v.x, v.y, v.z, v.w);
}

// 设置浮点数着色器变量
void Shader::setFloat(const std::string& name, float f) const {
    GLint loc = glGetUniformLocation(m_program, name.c_str());
    glUniform1f(loc, f);
}

// 设置整数着色器变量
void Shader::setInt(const std::string& name, int i) const {
    GLint loc = glGetUniformLocation(m_program, name.c_str());
    glUniform1i(loc, i);
}
