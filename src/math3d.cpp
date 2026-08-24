#include "math3d.h"

#include <math.h>

Vec3f Vec3f::operator-(const Vec3f &rhs) const
{
    Vec3f result = {x - rhs.x, y - rhs.y, z - rhs.z};
    return result;
}

Vec3f Vec3f::operator-() const
{
    Vec3f result = {-x, -y, -z};
    return result;
}

float Vec3f::dot(const Vec3f &rhs) const
{
    return x * rhs.x + y * rhs.y + z * rhs.z;
}

Vec3f Vec3f::cross(const Vec3f &rhs) const
{
    Vec3f result = {
        y * rhs.z - z * rhs.y,
        z * rhs.x - x * rhs.z,
        x * rhs.y - y * rhs.x
    };
    return result;
}

float Vec3f::length_squared() const
{
    return dot(*this);
}

int Vec3f::normalize()
{
    const float length_sq = length_squared();
    if (length_sq < 0.000001f) {
        return 0;
    }

    const float inverse_length = 1.0f / sqrtf(length_sq);
    x *= inverse_length;
    y *= inverse_length;
    z *= inverse_length;
    return 1;
}

Mat34f::Mat34f()
{
    set_identity();
}

void Mat34f::set_identity()
{
    m_[0][0] = 1.0f;
    m_[0][1] = 0.0f;
    m_[0][2] = 0.0f;
    m_[0][3] = 0.0f;
    m_[1][0] = 0.0f;
    m_[1][1] = 1.0f;
    m_[1][2] = 0.0f;
    m_[1][3] = 0.0f;
    m_[2][0] = 0.0f;
    m_[2][1] = 0.0f;
    m_[2][2] = 1.0f;
    m_[2][3] = 0.0f;
}

void Mat34f::set_view(const Vec3f &right, const Vec3f &up,
                      const Vec3f &backward, const Vec3f &eye)
{
    m_[0][0] = right.x;
    m_[0][1] = right.y;
    m_[0][2] = right.z;
    m_[0][3] = -right.dot(eye);
    m_[1][0] = up.x;
    m_[1][1] = up.y;
    m_[1][2] = up.z;
    m_[1][3] = -up.dot(eye);
    m_[2][0] = backward.x;
    m_[2][1] = backward.y;
    m_[2][2] = backward.z;
    m_[2][3] = -backward.dot(eye);
}

Vec3f Mat34f::transform_point(const Vec3f &point) const
{
    Vec3f result = {
        m_[0][0] * point.x + m_[0][1] * point.y + m_[0][2] * point.z + m_[0][3],
        m_[1][0] * point.x + m_[1][1] * point.y + m_[1][2] * point.z + m_[1][3],
        m_[2][0] * point.x + m_[2][1] * point.y + m_[2][2] * point.z + m_[2][3]
    };
    return result;
}
