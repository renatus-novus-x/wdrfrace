#ifndef WDR_MATH3D_H
#define WDR_MATH3D_H

#include <stdint.h>

class Vec3f {
public:
    float x;
    float y;
    float z;

    Vec3f operator-(const Vec3f &rhs) const;
    Vec3f operator-() const;
    float dot(const Vec3f &rhs) const;
    Vec3f cross(const Vec3f &rhs) const;
    float length_squared() const;
    int normalize();
};

struct Vec2s {
    int16_t x;
    int16_t y;
};

class Mat34f {
private:
    float m_[3][4];

public:
    Mat34f();
    void set_identity();
    void set_view(const Vec3f &right, const Vec3f &up,
                  const Vec3f &backward, const Vec3f &eye);
    Vec3f transform_point(const Vec3f &point) const;
};

#endif
