#include "game.h"

void camera_update() {
    auto *c = &Game->Camera;

    c->Velocity = math::lerp(c->Velocity, c->TargetVelocity, 0.7f);
    c->Position += c->Velocity * Memory->FrameDelta;

    c->Rotation = math::slerp(c->Rotation, c->TargetRotation, 0.3f);
    c->ViewMatrix = math::mul(math::rotation_matrix(c->Rotation), math::translation_matrix(-c->Position));
}

void camera::update_target_velocity_from_dirs() {
    float4 qy = math::rotation_quat(float3(0, 1, 0), -RotationEuler.y);

    float3 ns = math::qrot(qy, float3(0, 0, (f32) DirectionNS));
    float3 ew = math::qrot(qy, float3((f32) DirectionEW, 0, 0));

    TargetVelocity = (ns + ew + float3(0, (f32) DirectionUD, 0)) * MovementSpeed;
}

void camera::update_target_quat_from_euler() {
    float4 qx = math::rotation_quat(float3(1, 0, 0), RotationEuler.x);
    float4 qy = math::rotation_quat(float3(0, 1, 0), RotationEuler.y);
    float4 qz = math::rotation_quat(float3(0, 0, 1), RotationEuler.z);

    float4 q          = math::qmul(math::qmul(qx, qy), qz);
    TargetRotation = q;
}

camera::camera() {
    update_target_quat_from_euler();
}

void camera_init_perspective_matrix(f32 aspect) {
    Game->Camera.PerspectiveMatrix = math::perspective_matrix(60.0f * (f32) PI / 180, aspect, 0.1f, 1000.0f, math::pos_z);
}

bool camera_event(event e) {
    // This is called only when Game->MouseGrabbed is true

    auto *c = &Game->Camera;

    if (e.Type == event::Mouse_Moved) {
        f32 dx = -c->MouseSensitivity * e.DX;
        f32 dy = -c->MouseSensitivity * e.DY;
        
        c->RotationEuler.y = (f32) fmod(c->RotationEuler.y + dx, M_TAU);
        c->RotationEuler.x = clamp(c->RotationEuler.x + dy, -M_PI, M_PI);

        c->update_target_quat_from_euler();

        return true;
    }

    if (e.Type == event::Key_Pressed) {
        if (e.KeyCode == Key_W) {
            c->DirectionNS += 1;
        } else if (e.KeyCode == Key_A) {
            c->DirectionEW -= 1;
        } else if (e.KeyCode == Key_S) {
            c->DirectionNS -= 1;
        } else if (e.KeyCode == Key_D) {
            c->DirectionEW += 1;
        } else if (e.KeyCode == Key_LeftShift) {
            c->DirectionUD -= 1;
        } else if (e.KeyCode == Key_Space) {
            c->DirectionUD += 1;
        } else {
            return false;
        }

        c->update_target_velocity_from_dirs();
        // Should we return true here (that we handled the event, so stop propagating to other listeners)?
    }

    if (e.Type == event::Key_Released) {
        if (e.KeyCode == Key_W) {
            c->DirectionNS -= 1;
        } else if (e.KeyCode == Key_A) {
            c->DirectionEW += 1;
        } else if (e.KeyCode == Key_S) {
            c->DirectionNS += 1;
        } else if (e.KeyCode == Key_D) {
            c->DirectionEW -= 1;
        } else if (e.KeyCode == Key_LeftShift) {
            c->DirectionUD += 1;
        } else if (e.KeyCode == Key_Space) {
            c->DirectionUD -= 1;
        } else {
            return false;
        }

        c->update_target_velocity_from_dirs();
        // Should we return true here (that we handled the event, so stop propagating to other listeners)?
    }

    return false;
}

