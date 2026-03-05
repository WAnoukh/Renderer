#include "game.h"
#include "cglm/affine.h"
#include "cglm/cam.h"
#include "cglm/clipspace/view_lh.h"
#include "cglm/vec2.h"
#include "cglm/vec3.h"
#include "console/log.h"
#include "engine.h"
#include "game_runtime.h"
#include "rendering/shader.h"
#include "rendering/rendering.h"
#include "GLFW/glfw3.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_OPENGL3
#define CIMGUI_USE_GLFW
#include "cimgui.h"

void process_fps_camera(struct Game *game, struct FrameContext *frame, float aspect_ratio);
void draw_dummy_cube(struct Game *game, struct FrameContext *frame, GLuint shader);
void render_scene(struct Game *game, struct FrameContext *frame, vec3 cam_pos, mat4 vp);
void render_skybox(struct Game *game, struct FrameContext *frame, struct CameraFPS *cam);

const int SHADOW_WIDTH = 1024 * 2, SHADOW_HEIGHT = 1024 * 2;

void game_start(struct Game *game)
{
    game->camera = camera_get_default();
    game->camera.pos[2] = 1;


    load_cube_mesh();
    load_quad_mesh();

    game->light1 = (Light){
        .pos = {1,1,2},
        .color = {1, 1, 1},
        .intensity = 1,
    };

    glm_vec2_copy((vec2){3.8f, -0.44f}, game->sun);

    game->mat_brick = pbrmat_load(
    "resources/sprite/brick/Poliigon_BrickWallReclaimed_8320_BaseColor.jpg",
    "resources/sprite/brick/Poliigon_BrickWallReclaimed_8320_Normal.png",
    "resources/sprite/brick/Poliigon_BrickWallReclaimed_8320_Roughness.jpg",
    "resources/sprite/brick/Poliigon_BrickWallReclaimed_8320_Metallic.jpg",
    "resources/sprite/brick/Poliigon_BrickWallReclaimed_8320_AmbientOcclusion.jpg",
    0
    );

    game->mat_fabric = pbrmat_load(
    "resources/sprite/fabric/FabricPlainNaturalSheer009_COL_2K.jpg",
    "resources/sprite/fabric/FabricPlainNaturalSheer009_NRM_2K.png",
    "resources/sprite/fabric/FabricPlainNaturalSheer009_GLOSS_2K.jpg",
    0,
    0,
    "resources/sprite/fabric/FabricPlainNaturalSheer009_MASK_2K.png"
    );

    game->mat_gold = pbrmat_load(
    "resources/sprite/gold/Poliigon_MetalGoldPaint_7253_BaseColor.jpg",
    "resources/sprite/gold/Poliigon_MetalGoldPaint_7253_Normal.png",
    "resources/sprite/gold/Poliigon_MetalGoldPaint_7253_Roughness.jpg",
    "resources/sprite/gold/Poliigon_MetalGoldPaint_7253_Metallic.jpg",
    "resources/sprite/gold/Poliigon_MetalGoldPaint_7253_AmbientOcclusion.jpg",
    0
    );

    game->skybox = texture_load_hdr("resources/skybox/citrus_orchard_road_puresky_4k.hdr");

    glGenFramebuffers(1, &game->depthMapFBO);

    glGenTextures(1, &game->depthMap);
    glBindTexture(GL_TEXTURE_2D, game->depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindFramebuffer(GL_FRAMEBUFFER, game->depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, game->depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);  

    game->gbuffer = gbuffer_create(100, 100); 
    game->ping = pingpong_create(100, 100);
    game->pong = pingpong_create(100, 100);

    game->debug_mode = DM_NONE;

    game->ig_img_ref = ImTextureRef_ImTextureRef_TextureID(game->depthMap);

    game->fog_sample_count = 64;
    game->fog_strength = 2;
    glm_vec3_copy(game->fog_color, (vec3){0.5f, 0.6f, 0.7f});
}

void game_update(struct Game *game, struct FrameContext *frame)
{
    if(frame->window.is_framebuffer_resized)
    {
        int w = frame->window.width, h = frame->window.height;
        gbuffer_resize(&game->gbuffer, w, h);
        pingpong_resize(&game->ping, w, h);
        pingpong_resize(&game->pong, w, h);
        frame->window.is_framebuffer_resized = 0;
        LOG_INFO("Framebuffer resized.");
    }

    float aspect_ratio = (float)frame->window.width/(float)frame->window.height;

    if(is_key_pressed(frame->inputs , GLFW_KEY_GRAVE_ACCENT))
    {
        game->engine->show_console = !game->engine->show_console;
    }

    process_fps_camera(game, frame, aspect_ratio);

    struct CameraFPS *cam = &game->camera;
    Light *light = &game->light1;

    //render shadows
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, game->depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    mat4 light_proj;
    float ortho_size = 4;
    glm_ortho(-ortho_size, ortho_size, -ortho_size, ortho_size, 0.5f, 30, light_proj);
    mat4 light_view;
    vec3 light_dir = {
        cosf(game->sun[0]) * cosf(game->sun[1]),
        sinf(game->sun[0]) * cosf(game->sun[1]),
        sinf(game->sun[1])
    };
    glm_vec3_normalize(light_dir);
    vec3 center; glm_vec3_copy(GLM_VEC3_ZERO, center);
    vec3 lightPos, negDir;
    glm_vec3_negate_to(light_dir, negDir);
    glm_vec3_scale(negDir, 8, lightPos);
    glm_vec3_add(lightPos, center, lightPos);
    glm_look_lh(lightPos, light_dir, (vec3){0,0,1}, light_view);
    mat4 light_vp;
    glm_mat4_mul(light_proj, light_view, light_vp);
    render_scene(game, frame, light->pos, light_vp);
    //render_scene(game, frame, cam->pos, cam->vp);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    //REAL RENDER
    glBindFramebuffer(GL_FRAMEBUFFER, game->gbuffer.fbo);
    glViewport(0, 0, frame->window.width, frame->window.height);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClearDepth(1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);


    shader_set_mat4(shaders_use_pbr(), "sunVP", light_vp);
    shader_set_vec3(shaders_use_pbr(), "sunDir", negDir);

    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, game->depthMap);

    shader_set_texture(shaders_use_pbr(), "shadowMap", 10);


    glDepthMask(GL_FALSE);
    render_skybox(game, frame, cam);
    glDepthMask(GL_TRUE);

    render_scene(game, frame, cam->pos, cam->vp);

    //POST PROCESS
    {
        GLuint shader = shaders_use_filter_fog();
        glBindFramebuffer(GL_FRAMEBUFFER, game->ping.fbo);
        glViewport(0, 0, frame->window.width, frame->window.height);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, game->gbuffer.color); 

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, game->gbuffer.depth); 

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, game->depthMap); 

        mat4 camVP_inv;
        glm_mat4_inv_precise_sse2(cam->vp, camVP_inv);

        shader_set_float(shader, "fogStrength", game->fog_strength);
        shader_set_int(shader, "sampleCount", game->fog_sample_count);
        shader_set_vec3(shader, "fogColor", game->fog_color);

        shader_set_vec3(shader, "cameraPos", cam->pos);

        shader_set_mat4(shader, "camVP_inv", camVP_inv);
        shader_set_mat4(shader, "sunVP", light_vp);

        shader_set_float(shader, "near", cam->near);
        shader_set_float(shader, "far", cam->far);

        shader_set_texture(shader, "inShadowMap", 2);
        shader_set_texture(shader, "inDepth", 1);
        shader_set_texture(shader, "inColor", 0);

        glBindVertexArray(game->engine->rendering->quad_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    //RENDER GBUFFER TO SCREEN
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        GLuint shader = shaders_use_fullscreen();

        GLuint out_tex;

        if(is_key_pressed(frame->inputs, GLFW_KEY_1))
        {
            game->debug_mode = DM_NONE;
        }
        else if(is_key_pressed(frame->inputs, GLFW_KEY_2))
        {
            game->debug_mode = DM_NORMAL;
        }
        else if(is_key_pressed(frame->inputs, GLFW_KEY_3))
        {
            game->debug_mode = DM_DEPTH;
        }
        else if(is_key_pressed(frame->inputs, GLFW_KEY_4))
        {
            game->debug_mode = DM_ALBEDO;
        }

        switch(game->debug_mode)
        {
            default:
            case DM_NONE:
                out_tex = game->ping.color;
                break;
            case DM_NORMAL:
                out_tex = game->gbuffer.normal;
                break;
            case DM_DEPTH:
                out_tex = game->gbuffer.depth;
                break;
            case DM_ALBEDO:
                out_tex = game->gbuffer.albedo;
                break;
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, out_tex); 

        shader_set_texture(shader, "color", 0);

        glBindVertexArray(game->engine->rendering->quad_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    //
    //ImGUi

    igBegin("Settings", NULL, 0);
    char fps_text[100];
    sprintf(fps_text, "FPS:%f", 1/frame->real_dt);
    //igText(fps_text);

    igDragFloat2("sun", game->sun, 0.02f, -1000.0f, 1000.0f, "%.3f",0);
    int pixels = 500;
    ImVec2 size = (ImVec2){ pixels, pixels }; 

    ImVec2 uv0 = (ImVec2){0.0f, 1.0f};
    ImVec2 uv1 = (ImVec2){1.0f, 0.0f};

    igImage(*game->ig_img_ref, size, uv0, uv1);

    igDragFloat("Fog strength: ", &game->fog_strength, 0.003f, 0.0f, 1.2f, "%.3f", 0);
    igDragInt("Fog sampleCount: ", &game->fog_sample_count, 1, 10, 100, "%d", 0);
    igDragFloat3("Fog color: ", game->fog_color, 0.005f, 0.0f, 1.0f, "%.3f", 0);

    igEnd();
}

void render_skybox(struct Game *game, struct FrameContext *frame, struct CameraFPS *cam)
{
    mat4 model = GLM_MAT4_IDENTITY_INIT; 
    glm_scale(model, (vec3){20, 20, 20});

    GLuint shader = shaders_use_skybox();

    shader_set_mat4(shader, "vp", cam->vp);
    shader_set_mat4(shader, "model", model);

    shader_set_vec3(shader, "viewPos", cam->pos);


    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, game->skybox); 

    shader_set_texture(shader, "skybox", 5);
    
    draw_dummy_cube(game, frame, shader);
}

void render_scene(struct Game *game, struct FrameContext *frame, vec3 cam_pos, mat4 vp)
{
    Light *light = &game->light1;

    mat4 model = GLM_MAT4_IDENTITY_INIT;
    glm_translate_z(model, 1);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, game->skybox); 

    GLuint shader = shaders_use_pbr();

    shader_set_texture(shader, "skybox", 5);

    shader_set_pbrmat(shader, &game->mat_brick);
    shader_set_vec3(shader, "lightPos", game->light1.pos);
    shader_set_vec3(shader, "lightColor", light->color);
    shader_set_vec3(shader, "viewPos", cam_pos);

    shader_set_mat4(shader, "vp", vp);
    shader_set_mat4(shader, "model", model);

    draw_dummy_cube(game, frame, shader);


    glm_translate_x(model, 1.5f);
    shader_set_mat4(shader, "model", model);

    shader_set_pbrmat(shader, &game->mat_fabric);
    draw_dummy_cube(game, frame, shader);


    glm_translate_x(model, -3.0f);
    shader_set_mat4(shader, "model", model);

    shader_set_pbrmat(shader, &game->mat_gold);
    draw_dummy_cube(game, frame, shader);

    shader_set_pbrmat(shader, &game->mat_brick);
    glm_mat4_identity(model);
    glm_scale_make(model, (vec3){10, 10, 0.1f});

    /*
    shader = shaders_use_default();

    shader_set_vec3(shader, "color", (vec3){1,1,1});
    shader_set_float(shader, "alpha", 1);
    */

    mat4 mvp;
    glm_mat4_mul(vp, model, mvp);

    shader_set_mat4(shader, "model", model);

    draw_dummy_cube(game, frame, shader);
}

void process_fps_camera(struct Game *game, struct FrameContext *frame, float aspect_ratio)
{
    float input_x = (float)is_key_down(frame->inputs, GLFW_KEY_W) - (float)is_key_down(frame->inputs, GLFW_KEY_S);
    float input_y = (float)is_key_down(frame->inputs, GLFW_KEY_D) - (float)is_key_down(frame->inputs, GLFW_KEY_A);

    float inputs_pitch = (float)is_key_down(frame->inputs, GLFW_KEY_I) - (float)is_key_down(frame->inputs, GLFW_KEY_K);
    float inputs_yaw = (float)is_key_down(frame->inputs, GLFW_KEY_L) - (float)is_key_down(frame->inputs, GLFW_KEY_J);

    game->camera.pitch += inputs_pitch * 120 * frame->real_dt;
    game->camera.yaw += inputs_yaw * 120 * frame->real_dt;

    vec3 speed = {0, 0, 0};
    glm_vec3_muladds(game->camera.forward, input_x, speed);
    glm_vec3_muladds(game->camera.right, input_y, speed);

    glm_vec3_muladds(speed, frame->real_dt, game->camera.pos);

    camera_compute_vp(&game->camera, aspect_ratio);
}

void draw_dummy_cube(struct Game *game, struct FrameContext *frame, GLuint shader)
{
    shader_set_float(shader, "alpha", 1);

    glBindVertexArray(game->engine->rendering->cube_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void gbuffer_resize(GBuffer *gbuffer, int width, int height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &gbuffer->fbo);
    glDeleteTextures(1,&gbuffer->color);
    glDeleteTextures(1,&gbuffer->depth);
    glDeleteTextures(1,&gbuffer->albedo);
    glDeleteTextures(1,&gbuffer->normal);
    glDeleteTextures(1,&gbuffer->roughness);

    *gbuffer = gbuffer_create(width, height);
}

GBuffer gbuffer_create(int w, int h)
{
    GBuffer gbuffer;

    glGenFramebuffers(1, &gbuffer.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.fbo);

    glEnable(GL_DEPTH_TEST);

    glGenTextures(1, &gbuffer.color);
    glGenTextures(1, &gbuffer.depth);
    glGenTextures(1, &gbuffer.albedo);
    glGenTextures(1, &gbuffer.normal);
    glGenTextures(1, &gbuffer.roughness);

    glBindTexture(GL_TEXTURE_2D, gbuffer.color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gbuffer.color, 0);

    glBindTexture(GL_TEXTURE_2D, gbuffer.normal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gbuffer.normal, 0);

    glBindTexture(GL_TEXTURE_2D, gbuffer.albedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gbuffer.albedo, 0);

    glBindTexture(GL_TEXTURE_2D, gbuffer.depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gbuffer.depth, 0);

    const GLenum attachments[3] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2
    };
    glDrawBuffers(3, attachments);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG_ERROR("Framebuffer not complete!");
    }

    return gbuffer;
}

PingPongFBO pingpong_create(int w, int h)
{
    PingPongFBO pp;
    glGenFramebuffers(1, &pp.fbo);
    glGenTextures(1, &pp.color);

    glBindFramebuffer(GL_FRAMEBUFFER, pp.fbo);

    glBindTexture(GL_TEXTURE_2D, pp.color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pp.color, 0);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG_ERROR("Framebuffer not complete!");
    }

    return pp;
}

void pingpong_resize(PingPongFBO *pp, int width, int height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &pp->fbo);
    glDeleteTextures(1,&pp->color);

    *pp = pingpong_create(width, height);
}
