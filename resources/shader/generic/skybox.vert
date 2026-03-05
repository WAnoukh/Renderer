#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 4) in vec2 tPos;

uniform mat4 vp;
uniform mat4 model;

out VS_OUT
{
    vec2 TexCoord;
    vec3 FragPos;
} vs_out;


void main()
{
    vec4 worldPos = model * vec4(aPos, 1.);

    vs_out.FragPos = worldPos.xyz;
    vs_out.TexCoord = tPos;

    gl_Position = vp * worldPos;
}
