#version 330 core
layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aNormal;
layout (location=2) in vec2 aUV;

out vec3 vNormal;
out vec3 vWorldPos;
out vec2 vUV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 wp = model * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;

    mat3 nmat = transpose(inverse(mat3(model)));
    vNormal = normalize(nmat * aNormal);

    vUV = aUV;

    gl_Position = projection * view * wp;
}
