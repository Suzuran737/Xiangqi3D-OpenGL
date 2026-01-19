#version 330 core
// 线段片段着色器

uniform vec3 color;
out vec4 FragColor;

void main() {
    FragColor = vec4(color, 1.0);
}
