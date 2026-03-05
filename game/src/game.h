#ifndef GAME_H
#define GAME_H

#include "cglm/types.h"
#include "rendering/fps_camera.h"
#include "rendering/material.h"
#include "rendering/texture.h"

typedef struct GLFWwindow GLFWwindow;
struct GameArenas;
struct Engine;
struct InputInfo;
struct FrameContext;
struct ImTextureRef;

typedef struct Light
{
    vec3 pos;
    vec3 color;
    float intensity;
} Light;

enum Mode
{
    M_PLAYING,
    M_EDITOR_2D,
};

typedef struct GBuffer
{
    GLuint  fbo;
    GLuint  color;
    GLuint  depth;
    GLuint  albedo;
    GLuint  normal;
    GLuint  roughness;
} GBuffer;

typedef struct PingPongFBO
{
    GLuint  fbo;
    GLuint  color;
} PingPongFBO;

enum DebugMode
{
    DM_NONE,
    DM_NORMAL,
    DM_DEPTH,
    DM_ALBEDO,
};

typedef struct Game 
{
    struct CameraFPS    camera;

    struct GameArenas * arenas;
    struct TextureInfo *textures;
    struct Engine *     engine;

    Light               light1;
    PbrMaterial         mat_brick;
    PbrMaterial         mat_fabric;
    PbrMaterial         mat_gold;

    GLuint              skybox;

    vec2                sun;

    GLuint              depthMapFBO;
    GLuint              depthMap;

    GBuffer             gbuffer;
    PingPongFBO         ping;
    PingPongFBO         pong;

    int                 fog_sample_count;
    float               fog_strength;
    vec3                fog_color;

    enum DebugMode      debug_mode;

    struct ImTextureRef *ig_img_ref;
} Game;

void game_start(struct Game *game);

void game_update(struct Game *game, struct FrameContext *frame);

GBuffer gbuffer_create(int w, int h);

void gbuffer_resize(GBuffer *gbuffer, int width, int height);

PingPongFBO pingpong_create(int w, int h);

void pingpong_resize(PingPongFBO *pp, int width, int height);

#endif // GAME_H
