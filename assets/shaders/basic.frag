#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vUV;

out vec4 FragColor;

uniform vec3 lightDir;
uniform vec3 viewPos;
uniform vec3 baseColor;
uniform float alpha;

uniform bool useTexture;
uniform sampler2D albedoMap;
uniform bool useTextureAlpha;

uniform bool useNormalMap;
uniform sampler2D normalMap;

uniform bool useShadow;
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;

uniform float roughness;
uniform float metalness;

const float PI = 3.14159265;

vec3 toLinear(vec3 c) { return pow(c, vec3(2.2)); }
vec3 toGamma(vec3 c)  { return pow(c, vec3(1.0/2.2)); }

mat3 cotangentFrame(vec3 N, vec3 p, vec2 uv) {
    vec3 dp1 = dFdx(p);
    vec3 dp2 = dFdy(p);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);

    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    float invMax = inversesqrt(max(dot(T, T), dot(B, B)));
    return mat3(T * invMax, B * invMax, N);
}

vec3 getNormal() {
    vec3 N = normalize(vNormal);
    if (!useNormalMap) return N;
    vec3 mapN = texture(normalMap, vUV).xyz * 2.0 - 1.0;
    mat3 TBN = cotangentFrame(N, vWorldPos, vUV);
    return normalize(TBN * mapN);
}

float shadowFactor(vec3 N) {
    if (!useShadow) return 0.0;
    vec4 fragPosLight = lightSpaceMatrix * vec4(vWorldPos, 1.0);
    vec3 projCoords = fragPosLight.xyz / fragPosLight.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;

    float bias = max(0.002 * (1.0 - dot(N, normalize(-lightDir))), 0.0005);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    return shadow;
}

void main() {
    vec3 N = getNormal();
    vec3 V = normalize(viewPos - vWorldPos);
    vec3 L = normalize(-lightDir);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    vec3 albedo = baseColor;
    if (useTexture) {
        albedo = texture(albedoMap, vUV).rgb;
    }
    albedo = toLinear(albedo);
    float texAlpha = 1.0;
    if (useTexture && useTextureAlpha) {
        texAlpha = texture(albedoMap, vUV).a;
    }

    float r = clamp(roughness, 0.04, 1.0);
    float m = clamp(metalness, 0.0, 1.0);

    vec3 F0 = mix(vec3(0.04), albedo, m);

    float a = r * r;
    float a2 = a * a;
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    float D = a2 / (PI * denom * denom + 1e-7);

    float k = (r + 1.0);
    k = (k * k) / 8.0;
    float Gv = NdotV / (NdotV * (1.0 - k) + k);
    float Gl = NdotL / (NdotL * (1.0 - k) + k);
    float G = Gv * Gl;

    vec3 F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);

    vec3 spec = (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - m);
    vec3 diffuse = kD * albedo / PI;

    vec3 Lo = (diffuse + spec) * NdotL * 1.25;
    vec3 ambient = 0.5 * albedo;

    float shadow = shadowFactor(N);
    vec3 color = ambient + (1.0 - shadow) * Lo;

    color = toGamma(color);
    FragColor = vec4(color, alpha * texAlpha);
}
