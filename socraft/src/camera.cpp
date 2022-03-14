#include "game.h"

void camera_update() {
    auto *c = &Game->Camera;

    c->Velocity = math::lerp(c->Velocity, c->TargetVelocity, 0.1f);
    c->Position += c->Velocity * Memory->FrameDelta;

    c->Rotation = math::qslerp(c->Rotation, c->TargetRotation, 0.4f);
    
    c->update_target_velocity_from_dirs();

    c->ViewMatrix = math::mul(math::rotation_matrix(math::qconj(c->Rotation)), math::translation_matrix(-c->Position));
}

void camera::update_target_velocity_from_dirs() {
    float3 forward = math::qrot(Rotation, float3(0, 0, -1));
    
    // Get rid of y component so the camera doesn't fly up when looking downwards/upwards
    forward.y = 0;
    forward = math::normalize(forward);

    float3 right = { -forward.z, 0, forward.x};
    float3 up = float3(0, 1, 0);

    auto targetDir = forward * DirectionNS + right * DirectionEW + up * DirectionUD;
    TargetVelocity = targetDir * MovementSpeed; 
}

void camera::update_target_quat_from_euler() {
    float4 qx = math::rotation_quat(float3(1, 0, 0), -RotationEuler.x);
    float4 qy = math::rotation_quat(float3(0, 1, 0), -RotationEuler.y);
    float4 qz = math::rotation_quat(float3(0, 0, 1), -RotationEuler.z);
    TargetRotation = math::qmul(math::qmul(qz, qy), qx);
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
        
        // @TODO: Add a bit of space before limits (for aesthetic purposes)
        c->RotationEuler.y = (f32) fmod(c->RotationEuler.y + dx, M_TAU);
        c->RotationEuler.x = clamp(c->RotationEuler.x + dy, -M_PI, M_PI);

        c->update_target_quat_from_euler();

        return true;
    }

    if (e.Type == event::Key_Pressed) {
        if (e.KeyCode == Key_W) {
            c->DirectionNS -= 1;
        } else if (e.KeyCode == Key_A) {
            c->DirectionEW -= 1;
        } else if (e.KeyCode == Key_S) {
            c->DirectionNS += 1;
        } else if (e.KeyCode == Key_D) {
            c->DirectionEW += 1;
        } else if (e.KeyCode == Key_LeftShift) {
            c->DirectionUD -= 1;
        } else if (e.KeyCode == Key_Space) {
            c->DirectionUD += 1;
        } else {
            return false;
        }
        // Should we return true here (that we handled the event, so stop propagating to other listeners)?
    }

    if (e.Type == event::Key_Released) {
        if (e.KeyCode == Key_W) {
            c->DirectionNS += 1;
        } else if (e.KeyCode == Key_A) {
            c->DirectionEW += 1;
        } else if (e.KeyCode == Key_S) {
            c->DirectionNS -= 1;
        } else if (e.KeyCode == Key_D) {
            c->DirectionEW -= 1;
        } else if (e.KeyCode == Key_LeftShift) {
            c->DirectionUD += 1;
        } else if (e.KeyCode == Key_Space) {
            c->DirectionUD -= 1;
        } else {
            return false;
        }
        // Should we return true here (that we handled the event, so stop propagating to other listeners)?
    }

    return false;
}

