#version 330 core
// 文字片段着色器

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D text;
uniform vec3 textColor;

void main() {
    float a = texture(text, TexCoords).r;
    FragColor = vec4(textColor, a);
}
