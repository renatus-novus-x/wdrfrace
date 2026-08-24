#ifndef WDR_CAMERA_H
#define WDR_CAMERA_H

#include "math3d.h"

class Camera {
 private:
    Mat34f view_;

 public:
  Camera();
  int look_at(const Vec3f &eye, const Vec3f &target, const Vec3f &up);
  Vec3f world_to_view(const Vec3f &point) const;
};

#endif
