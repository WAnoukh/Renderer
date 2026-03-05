#ifndef RENDERING_H
#define RENDERING_H
#include <cglm/ivec2.h>

typedef unsigned int GLuint;
struct Entity;
struct TextureAtlas;
struct Camera;

struct Rendering
{
    struct Camera *main_camera;

    GLuint shader_program_default;
    GLuint shader_program_textured;
    GLuint shader_program_fog;
    GLuint shader_program_pbr;
    GLuint shader_program_skybox;
    GLuint shader_program_fullscreen;
    GLuint shader_program_filter_fog;

    mat3 screen_space_view;

    unsigned int cube_vbo, cube_vao;
    unsigned int quad_vbo, quad_vao;
};

void rendering_set_current(struct Rendering *info);

struct Rendering *rendering_get_current();

void rendering_set_camera(struct Camera *new_main_camera);

void rendering_defaults(struct Rendering *rendering);

void rendering_start(struct Rendering *rendering);

void load_default_shaders();

unsigned int shaders_use_default();

unsigned int shaders_use_fog();

unsigned int shaders_use_textured();

unsigned int shaders_use_pbr();

unsigned int shaders_use_skybox();

unsigned int shaders_use_fullscreen();

unsigned int shaders_use_filter_fog();

void load_cube_mesh();

void load_quad_mesh();

#endif //RENDERING_H
