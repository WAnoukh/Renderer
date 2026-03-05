#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;

out VS_OUT
{
    vec2 TexCoord;
} vs_out;

void main()
{
    vs_out.TexCoord = aUV;

    vec4 worldPos =  ;

    vs_out

    gl_Position = vec4(aPos * 2, 1);
}
