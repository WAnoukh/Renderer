#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTangent;
layout (location = 3) in vec3 aBitangent;
layout (location = 4) in vec2 aUV;

uniform mat4 model;
uniform mat4 vp;
uniform mat4 sunVP;

out VS_OUT
{
    vec2 TexCoord;
    vec3 FragPos;
    vec4 FragPosSunSpace;
    mat3 TBN;
} vs_out;

void main()
{
    vs_out.TexCoord = aUV;

    vec4 worldPos = model * vec4(aPos, 1.);
    
    vs_out.FragPos = worldPos.xyz;

    vec3 T =  normalize(mat3(model) * aTangent);
    vec3 B =  normalize(mat3(model) * aBitangent);
    vec3 N =  normalize(mat3(model) * aNormal);

    vs_out.TBN = mat3(T,B,N);

    vs_out.TexCoord = aUV;

    vs_out.FragPosSunSpace = sunVP * vec4(vs_out.FragPos, 1.0);

    gl_Position = vp * worldPos;
}
