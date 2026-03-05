#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform vec3 color;
uniform float alpha;
uniform sampler2D texture;


void main()
{
    FragColor = texture2D(texture, TexCoord); 
    if(FragColor.a == 0) discard;
}
