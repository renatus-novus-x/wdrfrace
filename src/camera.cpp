#include "camera.h"

Camera::Camera()
{
}

int Camera::look_at(const Vec3f &eye, const Vec3f &target, const Vec3f &up)
{
    Vec3f forward = target - eye;
    if (!forward.normalize()) {
        return 0;
    }

    Vec3f right = forward.cross(up);
    if (!right.normalize()) {
        return 0;
    }

    Vec3f camera_up = right.cross(forward);
    const Vec3f backward = -forward;
    view_.set_view(right, camera_up, backward, eye);
    return 1;
}

Vec3f Camera::world_to_view(const Vec3f &point) const
{
    return view_.transform_point(point);
}
