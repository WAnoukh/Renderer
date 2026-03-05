#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform int sampleCount;
uniform float fogStrength;
uniform vec3 fogColor;

uniform mat4 sunVP;
uniform mat4 camVP_inv;
uniform vec2 screenSize;
uniform vec3 cameraPos;

uniform sampler2D inColor;
uniform sampler2D inDepth;
uniform sampler2D inShadowMap;

uniform float far;
uniform float near;

float linearize_depth(float depth, float near, float far)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));
}

float SunShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0)
    {
        return 0.0;
    }

    float closestDepth = texture(inShadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = 0.001;
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

    return shadow;
}

vec3 fragCoord_to_worldPos(vec2 uv, float depth)
{
    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    vec4 world = camVP_inv * clip;
    world /= world.w;

    return world.xyz;
}

float ray_march_volumetric(int iterations, vec3 startPos, vec3 endPos)
{
    if (iterations < 2) return 0.0;

    float accum = 0.0;
    float dist = distance(startPos, endPos);
    float stepLen = dist / float(iterations);

    for (int i = 0; i < iterations; ++i)
    {
        float t = float(i) / float(iterations - 1);
        vec3 worldPos = mix(startPos, endPos, t);

        vec4 lightSpace = sunVP * vec4(worldPos, 1.0);
        float shadow = SunShadowCalculation(lightSpace);

        float lit = 1.0 - shadow;

        accum += lit * stepLen;
    }

    return accum;
}

vec3 reconstruct_world_dir(vec2 uv)
{
    vec4 clip = vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    vec4 world = camVP_inv * clip;
    world /= world.w;
    return normalize(world.xyz - cameraPos);
}

void main()
{
    float depth = texture(inDepth, TexCoord).r;
    vec3 color = texture(inColor, TexCoord).rgb;

    vec3 endPos;

    if (depth < 1.0)
    {
        endPos = fragCoord_to_worldPos(TexCoord, depth);
    }
    else
    {
        vec3 viewDir = reconstruct_world_dir(TexCoord);
        endPos = cameraPos + viewDir * 10;
    }

    float opticalDepth = ray_march_volumetric(sampleCount, cameraPos, endPos);
    float fogAmount = 1.0 - exp(-opticalDepth * fogStrength);

    vec3 finalColor = color + fogColor * fogAmount;
    FragColor = vec4(finalColor, 1.0);
}
