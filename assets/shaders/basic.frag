#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vUV;

out vec4 FragColor;

uniform vec3 lightDir;
uniform vec3 viewPos;
uniform vec3 baseColor;
uniform float alpha;
uniform float specularStrength;
uniform float shininess;

uniform bool useTexture;
uniform sampler2D albedoMap;

vec3 toLinear(vec3 c) { return pow(c, vec3(2.2)); }
vec3 toGamma(vec3 c)  { return pow(c, vec3(1.0/2.2)); }

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-lightDir);
    vec3 V = normalize(viewPos - vWorldPos);
    vec3 H = normalize(L + V);

    float ndl = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), shininess);

    vec3 albedo = baseColor;
    if (useTexture) {
        vec3 tex = texture(albedoMap, vUV).rgb;
        albedo = tex;
    }

    // simple lighting: ambient + diffuse
    vec3 amb = 0.35 * albedo;              // ✅增强环境光（立刻不那么“闷”）
    vec3 dif = (0.85 * ndl) * albedo;
    vec3 spe = specularStrength * spec * vec3(1.0);

    vec3 color = amb + dif + spe;

    // gamma output (cheap)
    color = toGamma(toLinear(color));

    FragColor = vec4(color, alpha);
}
