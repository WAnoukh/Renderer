#include <cglm/cglm.h>
#include <stdio.h>

#include "glad/glad.h"
#include "rendering/shader.h"
#include "texture.h"
#include "rendering/rendering.h"

#define GLCheckError() GLCheckErrorImpl(__FILE__, __LINE__)

struct Rendering *current_rendering = NULL;

static void GLCheckErrorImpl(const char* file, int line) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        const char* error;
        switch (err) {
            case GL_INVALID_ENUM:                  error = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "GL_INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:                error = "GL_STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:               error = "GL_STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "GL_OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            default:                               error = "Unknown Error"; break;
        }
        fprintf(stderr, "OpenGL error (%s) in %s:%d\n", error, file, line);
    }
}

void rendering_set_current(struct Rendering *info)
{
    current_rendering = info;
}

struct Rendering *rendering_get_current()
{
    return current_rendering;
}

void load_default_shaders()
{
    assert(current_rendering);


    current_rendering->shader_program_default = create_shader_program(
            "resources/shader/generic/default.vert", 
            "resources/shader/generic/default.frag"
            );
    if (!current_rendering->shader_program_default)
    {
        exit(1);
    }


    current_rendering->shader_program_fog = create_shader_program(
            "resources/shader/generic/default.vert", 
            "resources/shader/generic/fog.frag"
            );
    if (!current_rendering->shader_program_fog)
    {
        exit(1);
    }


    current_rendering->shader_program_textured = create_shader_program(
            "resources/shader/generic/default.vert", 
            "resources/shader/generic/textured.frag"
            );
    if (!current_rendering->shader_program_textured)
    {
        exit(1);
    }


    current_rendering->shader_program_pbr = create_shader_program(
            "resources/shader/generic/pbr.vert", 
            "resources/shader/generic/pbr.frag"
            );
    if (!current_rendering->shader_program_pbr)
    {
        exit(1);
    }

    current_rendering->shader_program_skybox = create_shader_program(
            "resources/shader/generic/skybox.vert", 
            "resources/shader/generic/skybox.frag"
            );
    if (!current_rendering->shader_program_skybox)
    {
        exit(1);
    }

    current_rendering->shader_program_fullscreen = create_shader_program(
            "resources/shader/generic/fullscreen.vert", 
            "resources/shader/generic/fullscreen.frag"
            );
    if (!current_rendering->shader_program_fullscreen)
    {
        exit(1);
    }

    current_rendering->shader_program_filter_fog = create_shader_program(
            "resources/shader/generic/fullscreen.vert", 
            "resources/shader/generic/filter_fog.frag"
            );
    if (!current_rendering->shader_program_filter_fog)
    {
        exit(1);
    }
}

void create_screen_space_view(mat3 out_view)
{
    glm_mat3_zero(out_view);
    out_view[0][0] = 2;
    out_view[1][1] = 2;
    out_view[2][0] = -1;
    out_view[2][1] = -1;
}

void rendering_set_camera(struct Camera *new_main_camera)
{
    assert(current_rendering);
    assert(new_main_camera);
    current_rendering->main_camera = new_main_camera;
}

void rendering_defaults(struct Rendering *rendering)
{
    *rendering = (struct Rendering){0};
    create_screen_space_view(rendering->screen_space_view);
}

void rendering_start(struct Rendering *rendering)
{
    load_default_shaders();
}

unsigned int shaders_use_default()
{
    assert(current_rendering);
    assert(current_rendering->shader_program_default);
    glUseProgram(current_rendering->shader_program_default);
    return current_rendering->shader_program_default;
}

unsigned int shaders_use_fog()
{
    assert(current_rendering);
    assert(current_rendering->shader_program_fog);
    glUseProgram(current_rendering->shader_program_fog);
    return current_rendering->shader_program_fog;
}

unsigned int shaders_use_textured()
{
    assert(current_rendering);
    assert(current_rendering->shader_program_textured);
    glUseProgram(current_rendering->shader_program_textured);
    return current_rendering->shader_program_textured;
}

unsigned int shaders_use_pbr()
{
    assert(current_rendering);
    assert(current_rendering->shader_program_pbr);
    glUseProgram(current_rendering->shader_program_pbr);
    return current_rendering->shader_program_pbr;
}

unsigned int shaders_use_skybox()
{
    assert(current_rendering);
    assert(current_rendering->shader_program_skybox);
    glUseProgram(current_rendering->shader_program_skybox);
    return current_rendering->shader_program_skybox;
}

unsigned int shaders_use_fullscreen()
{
    assert(current_rendering);
    assert(current_rendering->shader_program_fullscreen);
    glUseProgram(current_rendering->shader_program_fullscreen);
    return current_rendering->shader_program_fullscreen;
}

unsigned int shaders_use_filter_fog()
{
    assert(current_rendering);
    assert(current_rendering->shader_program_filter_fog);
    glUseProgram(current_rendering->shader_program_filter_fog);
    return current_rendering->shader_program_filter_fog;
}

const float cube_vertices[] = {

    // -Z (back): T=+X, B=+Y
    -0.5f,-0.5f,-0.5f,  0,0,-1,   1,0,0,   0,1,0,   0,0,
     0.5f,-0.5f,-0.5f,  0,0,-1,   1,0,0,   0,1,0,   1,0,
     0.5f, 0.5f,-0.5f,  0,0,-1,   1,0,0,   0,1,0,   1,1,
     0.5f, 0.5f,-0.5f,  0,0,-1,   1,0,0,   0,1,0,   1,1,
    -0.5f, 0.5f,-0.5f,  0,0,-1,   1,0,0,   0,1,0,   0,1,
    -0.5f,-0.5f,-0.5f,  0,0,-1,   1,0,0,   0,1,0,   0,0,

    // +Z (front): T=+X, B=+Y
    -0.5f,-0.5f, 0.5f,  0,0, 1,   1,0,0,   0,1,0,   0,0,
     0.5f,-0.5f, 0.5f,  0,0, 1,   1,0,0,   0,1,0,   1,0,
     0.5f, 0.5f, 0.5f,  0,0, 1,   1,0,0,   0,1,0,   1,1,
     0.5f, 0.5f, 0.5f,  0,0, 1,   1,0,0,   0,1,0,   1,1,
    -0.5f, 0.5f, 0.5f,  0,0, 1,   1,0,0,   0,1,0,   0,1,
    -0.5f,-0.5f, 0.5f,  0,0, 1,   1,0,0,   0,1,0,   0,0,

    // -X (left):  T=+Y, B=-Z   (THIS is the fix)
    -0.5f, 0.5f, 0.5f, -1,0,0,   0,1,0,   0,0,-1,  1,0,
    -0.5f, 0.5f,-0.5f, -1,0,0,   0,1,0,   0,0,-1,  1,1,
    -0.5f,-0.5f,-0.5f, -1,0,0,   0,1,0,   0,0,-1,  0,1,
    -0.5f,-0.5f,-0.5f, -1,0,0,   0,1,0,   0,0,-1,  0,1,
    -0.5f,-0.5f, 0.5f, -1,0,0,   0,1,0,   0,0,-1,  0,0,
    -0.5f, 0.5f, 0.5f, -1,0,0,   0,1,0,   0,0,-1,  1,0,

    // +X (right): T=+Y, B=-Z   (matches your UVs; handedness differs vs N, but this is correct for UV)
     0.5f, 0.5f, 0.5f,  1,0,0,   0,1,0,   0,0,-1,  1,0,
     0.5f, 0.5f,-0.5f,  1,0,0,   0,1,0,   0,0,-1,  1,1,
     0.5f,-0.5f,-0.5f,  1,0,0,   0,1,0,   0,0,-1,  0,1,
     0.5f,-0.5f,-0.5f,  1,0,0,   0,1,0,   0,0,-1,  0,1,
     0.5f,-0.5f, 0.5f,  1,0,0,   0,1,0,   0,0,-1,  0,0,
     0.5f, 0.5f, 0.5f,  1,0,0,   0,1,0,   0,0,-1,  1,0,

    // -Y (bottom): T=+X, B=-Z
    -0.5f,-0.5f,-0.5f,  0,-1,0,   1,0,0,   0,0,-1,  0,1,
     0.5f,-0.5f,-0.5f,  0,-1,0,   1,0,0,   0,0,-1,  1,1,
     0.5f,-0.5f, 0.5f,  0,-1,0,   1,0,0,   0,0,-1,  1,0,
     0.5f,-0.5f, 0.5f,  0,-1,0,   1,0,0,   0,0,-1,  1,0,
    -0.5f,-0.5f, 0.5f,  0,-1,0,   1,0,0,   0,0,-1,  0,0,
    -0.5f,-0.5f,-0.5f,  0,-1,0,   1,0,0,   0,0,-1,  0,1,

    // +Y (top): T=+X, B=-Z
    -0.5f, 0.5f,-0.5f,  0, 1,0,   1,0,0,   0,0,-1,  0,1,
     0.5f, 0.5f,-0.5f,  0, 1,0,   1,0,0,   0,0,-1,  1,1,
     0.5f, 0.5f, 0.5f,  0, 1,0,   1,0,0,   0,0,-1,  1,0,
     0.5f, 0.5f, 0.5f,  0, 1,0,   1,0,0,   0,0,-1,  1,0,
    -0.5f, 0.5f, 0.5f,  0, 1,0,   1,0,0,   0,0,-1,  0,0,
    -0.5f, 0.5f,-0.5f,  0, 1,0,   1,0,0,   0,0,-1,  0,1
};

const float quad_vertices[] = {
        -0.5f, -0.5f, 0,        0, 0, 
        -0.5f,  0.5f, 0,        0, 1,
         0.5f,  0.5f, 0,        1, 1,

         0.5f,  0.5f, 0,        1, 1,
        -0.5f, -0.5f, 0,        0, 0, 
         0.5f, -0.5f, 0,        1, 0,
};

void load_cube_mesh()
{
    assert(current_rendering);

    glGenVertexArrays(1, &current_rendering->cube_vao);
    glGenBuffers(1, &current_rendering->cube_vbo);

    glBindVertexArray(current_rendering->cube_vao);

    glBindBuffer(GL_ARRAY_BUFFER, current_rendering->cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // tangent attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    // bitangent attribute
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(9 * sizeof(float)));
    glEnableVertexAttribArray(3);
    // texture coord attribute
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(12 * sizeof(float)));
    glEnableVertexAttribArray(4);
}

void load_quad_mesh()
{
    assert(current_rendering);

    glGenVertexArrays(1, &current_rendering->quad_vao);
    glGenBuffers(1, &current_rendering->quad_vbo);

    glBindVertexArray(current_rendering->quad_vao); 

    glBindBuffer(GL_ARRAY_BUFFER, current_rendering->quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

/*
void draw_transformed_quad(unsigned int program, mat3 transform, vec3 color, float alpha)
{
    assert(current_rendering);
    assert(current_rendering->main_camera);
    mat3 result;
    glm_mat3_mul(current_rendering->main_camera->view, transform, result);
    //glm_mat3_mul(transform, camera->view, result);
    shader_set_mat3(program, "view", result);
    shader_set_vec3(program, "color", color);
    shader_set_float(program, "alpha", alpha);
    draw_quad();
}

void draw_transformed_quad_screen_space(unsigned int program, mat3 transform, vec3 color, float alpha)
{
    assert(current_rendering);
    mat3 result;
    glm_mat3_mul(current_rendering->screen_space_view, transform, result);
    //glm_mat3_mul(transform, camera->view, result);
    shader_set_mat3(program, "view", result);
    shader_set_vec3(program, "color", color);
    shader_set_float(program, "alpha", alpha);
    draw_quad();
}
*/
