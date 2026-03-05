#version 330 core

in VS_OUT
{
    vec2 TexCoord;
    vec3 FragPos;
    vec4 FragPosSunSpace;
    mat3 TBN;
} fs_in;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedo;

uniform vec3 color;
uniform float alpha;

uniform vec3    viewPos;

uniform vec3    sunDir;

uniform sampler2D   albedoMap;
uniform sampler2D   normalMap;
uniform sampler2D   roughMap;
uniform sampler2D   metallicMap;
uniform sampler2D   aoMap;
uniform sampler2D   alphaMap;

uniform sampler2D   shadowMap;

uniform sampler2D   skybox;

uniform bool        metallicEnable;
uniform bool        aoEnable;
uniform bool        alphaEnable;

vec2 dirToEquirectUV(vec3 d)
{
    d = normalize(d);

    float u = atan(d.y, d.x) / (2.0 * 3.14159265) + 0.5;
    float v = asin(d.z) / 3.14159265 + 0.5;

    return vec2(u, v);
}

float SunShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
    projCoords.y < 0.0 || projCoords.y > 1.0 ||
    projCoords.z < 0.0 || projCoords.z > 1.0)
    return 0.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;  

    float currentDepth = projCoords.z;

    float bias = 0.05;
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;

    return shadow;
}

void main()
{
    vec3 lightColor = vec3(1,1,1);

    vec3 normalTex = texture(normalMap, fs_in.TexCoord).rgb;
    vec3 albedo = texture(albedoMap, fs_in.TexCoord).rgb * (alphaEnable ? texture(alphaMap, fs_in.TexCoord).x : 1);
    float roughness = texture(roughMap, fs_in.TexCoord).r;
    float metallic = (metallicEnable ? texture(metallicMap, fs_in.TexCoord).r : 0);
    float ao = aoEnable ? texture(aoMap, fs_in.TexCoord).r : 1;

    normalTex = normalTex * 2.0 - 1.0; 

    vec3 N = normalize(fs_in.TBN * normalTex);
    vec3 L = normalize(sunDir);
    vec3 V = normalize(viewPos  - fs_in.FragPos);
    vec3 R = reflect(-V, N);

    vec3 env = texture(skybox, dirToEquirectUV(R)).rgb;  

    float diff = max(dot(N, L), 0.0);

    float r  = clamp(roughness, 0.04, 1.0);
    float shininess = mix(128.0, 4.0, r);          // smooth->tight, rough->wide
    float specStrength = mix(0.4, 0.09, r);  
    
    vec3 H = normalize(L + V);                     // Blinn half-vector
    float spec = pow(max(dot(N, H), 0.0), shininess) * specStrength;

    vec3 reflection = env * albedo;
    float reflAmount = metallic * (1.0 - roughness);

    vec3 ambient = 0.2f * albedo;
    vec3 diffuse = diff * albedo * lightColor * (1 - metallic);
    vec3 specCol = spec * lightColor;

    float shadow = SunShadowCalculation(fs_in.FragPosSunSpace);

    outNormal = vec4(N/2 + 0.5, 1);
    outAlbedo = vec4(albedo, 1);
    outColor = vec4((ambient + (diffuse + specCol) * (1 - shadow) + (reflection * reflAmount)) * ao, alpha);
}

