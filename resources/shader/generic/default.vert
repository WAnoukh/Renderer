#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 tPos;

uniform mat4 view;

out vec2 TexCoord;

void main()
{
    vec4 pos = vec4(aPos, 1.);
    pos = view * pos;
    gl_Position = pos;
    TexCoord = tPos;
}
