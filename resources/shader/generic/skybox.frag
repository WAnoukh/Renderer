#version 330 core

in VS_OUT
{
    vec2 TexCoord;
    vec3 FragPos;
} fs_in;

out vec4 FragColor;

uniform sampler2D skybox;

uniform vec3 viewPos;

vec2 dirToEquirectUV(vec3 d)
{
    d = normalize(d);
    float u = atan(d.y, d.x) * (1.0 / (2.0 * 3.14159265)) + 0.5;
    float v = asin(d.z)      * (1.0 / 3.14159265)        + 0.5;
    return vec2(u, v);
}

void main()
{
    vec3 dir = fs_in.FragPos - viewPos;              // direction from camera
    vec2 uv  = dirToEquirectUV(dir);

    FragColor = vec4(texture(skybox, uv).xyz / 3f, 1);
}
