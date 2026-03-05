#include "fps_camera.h"
#include "cglm/clipspace/persp_lh_no.h"
#include "cglm/clipspace/view_lh.h"
#include <math.h>

struct CameraFPS camera_get_default()
{
    return (struct CameraFPS){
        .pos = {0, 0, 0},
        .yaw = 0,
        .pitch = 0,
        .fov = 90,
        .near = 0.1f,
        .far = 100.f,
    };
}

void camera_compute_view(struct CameraFPS *camera)
{
    camera->pitch = glm_clamp(camera->pitch, -89, 89);
    camera->yaw = fmodf(camera->yaw, 360.f);

    float pitch = glm_rad(camera->pitch);
    float yaw = glm_rad(camera->yaw);

    vec3 dir = {};
    dir[0] = cosf(yaw) * cosf(pitch);
    dir[1] = sinf(yaw) * cosf(pitch);
    dir[2] =                     sinf(pitch);

    glm_vec3_normalize(dir);

    glm_look_lh(camera->pos, dir, (vec3){0,0,1}, camera->view);

    camera->forward[0] = cosf(yaw);
    camera->forward[1] = sinf(yaw);
    camera->forward[2] = 0;

    camera->right[0] = -sinf(yaw);
    camera->right[1] = cosf(yaw);
    camera->right[2] = 0;
}

void camera_compute_vp(struct CameraFPS *camera, float aspect_ratio)
{
    camera_compute_view(camera);
    mat4 proj;
    glm_perspective_lh_no(glm_rad(90), aspect_ratio, camera->near, camera->far, proj);
    glm_mat4_mul(proj, camera->view, camera->vp);
}
