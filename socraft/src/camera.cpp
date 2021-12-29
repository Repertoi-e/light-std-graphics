#include "game.h"

void camera_update() {
    auto *c = &Game->Camera;

    float4 qx = math::rotation_quat(float3(1, 0, 0), c->Rotation.x);
    float4 qy = math::rotation_quat(float3(0, 1, 0), c->Rotation.y);
    float4 qz = math::rotation_quat(float3(0, 0, 1), c->Rotation.z);

    float4 q = math::qmul(math::qmul(qx, qy), qz);

    c->ViewMatrix = math::mul(math::rotation_matrix(q), math::translation_matrix(-c->Position));
}

void camera_init_perspective_matrix(f32 aspect) {
    Game->Camera.PerspectiveMatrix = math::perspective_matrix(60.0f * (f32) PI / 180, aspect, 0.1f, 1000.0f, math::pos_z);
}
