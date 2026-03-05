#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D color;

void main()
{
    vec4 color = texture(color, TexCoord);
    FragColor = color;
}
