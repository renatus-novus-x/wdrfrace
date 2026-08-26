#include "car.h"

#include "screen.h"

namespace {

const int FIELD_W = 512;
const int FIELD_H = 480;
const int ANGLE_LIMIT = 65536;
const int NORMAL_MAX_SPEED = 320;
const int BOOST_MAX_SPEED = 512;
const int ACCELERATION = 8;
const int BOOST_ACCELERATION = 32;
const int BOOST_RELEASE_DECELERATION = 12;
const int DECELERATION = 6;
const int BRAKE_DECELERATION = 20;
const int COAST_DECELERATION = 2;
const int LANE_STEP = 6;
const int LANE_LIMIT = 64;
const int BOOST_LIMIT = 1000;
const int BOOST_COST = 8;
const float TRACK_RADIUS = 7.0f;
const float LANE_SCALE = 0.01875f;
const float CAR_HALF_LENGTH = 0.48f;
const float CAR_HALF_WIDTH = 0.28f;
const float CAR_BODY_HEIGHT = 0.22f;
const float CAR_ROOF_FRONT_SCALE = 0.46f;
const float CAR_ROOF_REAR_SCALE = 0.60f;
const float CAR_ROOF_WIDTH_SCALE = 0.68f;
const float CAR_ROOF_FRONT_HEIGHT = 0.62f;
const float CAR_ROOF_REAR_HEIGHT = 0.54f;
const float PROJECTION_SCALE = 300.0f;
const iocs_color_t COLOR_BLACK = 0x0000;

void draw_line(int x0, int y0, int x1, int y1, iocs_color_t color) {
  screen_line(x0, y0, x1, y1, color);
}

}  // namespace

void Car::initialize(int angle, int offset, int course_id) {
  angle_ = angle;
  offset_ = offset;
  course_id_ = course_id;
  speed_ = 0;
  boost_ = BOOST_LIMIT;
  boosting_ = 0;
  drift_ = 0;
  lap_ = 0;
  for (int page = 0; page < 2; ++page) {
    have_previous_[page] = 0;
    for (int i = 0; i < VERTEX_COUNT; ++i) previous_visible_[page][i] = 0;
  }
  for (int i = 0; i < VERTEX_COUNT; ++i) {
    current_visible_[i] = 0;
  }
}

void Car::update(const CarInput &input) {
  drift_ = 0;
  if (input.left && !input.right) drift_ = -1;
  if (input.right && !input.left) drift_ = 1;
  if (input.accelerate) speed_ += ACCELERATION;
  if (input.decelerate) speed_ -= DECELERATION;
  if (input.brake) speed_ -= BRAKE_DECELERATION;
  boosting_ = input.boost && boost_ > 0;
  if (boosting_) {
    speed_ += BOOST_ACCELERATION;
    boost_ -= BOOST_COST;
    if (boost_ < 0) boost_ = 0;
  }
  if (!input.accelerate && !boosting_ && speed_ > 0) {
    speed_ -= COAST_DECELERATION;
  }
  if (!boosting_ && speed_ > NORMAL_MAX_SPEED) {
    speed_ -= BOOST_RELEASE_DECELERATION;
    if (!input.decelerate && !input.brake && speed_ < NORMAL_MAX_SPEED) {
      speed_ = NORMAL_MAX_SPEED;
    }
  }
  if (speed_ < 0) speed_ = 0;
  if (boosting_ && speed_ > BOOST_MAX_SPEED) speed_ = BOOST_MAX_SPEED;
  if (!boosting_ && speed_ > BOOST_MAX_SPEED) speed_ = BOOST_MAX_SPEED;
  if (!boosting_ && speed_ > NORMAL_MAX_SPEED &&
      input.decelerate == 0 && input.brake == 0) {
    /* Preserve a short over-speed coast instead of hard-clamping to 320. */
  }

  if (input.left) offset_ -= LANE_STEP;
  if (input.right) offset_ += LANE_STEP;
  if (offset_ < -LANE_LIMIT) offset_ = -LANE_LIMIT;
  if (offset_ > LANE_LIMIT) offset_ = LANE_LIMIT;

  angle_ += speed_;
  while (angle_ >= ANGLE_LIMIT) {
    angle_ -= ANGLE_LIMIT;
    ++lap_;
  }
}

int Car::project(const Camera &camera, const Vec3f &point,
                 Vec2s &screen) const {
  const Vec3f view = camera.world_to_view(point);
  if (view.z > -1.0f) return 0;
  const float scale = PROJECTION_SCALE / -view.z;
  const int x = (int)(FIELD_W * 0.5f + view.x * scale);
  const int y = (int)(FIELD_H * 0.5f - view.y * scale);
  if (x < 0 || x >= FIELD_W || y < 40 || y >= FIELD_H - 24) return 0;
  screen.x = (int16_t)x;
  screen.y = (int16_t)y;
  return 1;
}

void Car::prepare_render(const Camera &camera, const float *sin_table) {
  const int index = (angle_ >> 8) & (TRIG_TABLE_SIZE - 1);
  const float s = sin_table[index];
  const float c = sin_table[(index + TRIG_TABLE_SIZE / 4)
                            & (TRIG_TABLE_SIZE - 1)];
  const float radius = TRACK_RADIUS + offset_ * LANE_SCALE;
  float center_x = s * radius;
  float center_z = c * radius;
  if (course_id_ == 1) {
    center_x *= 1.12f;
    center_z *= 0.82f;
  } else if (course_id_ == 2) {
    const float cos3 = 4.0f * c * c * c - 3.0f * c;
    const float scale = 1.0f + 0.11f * cos3;
    center_x *= scale;
    center_z *= scale;
  }
  const float side_x = s * CAR_HALF_WIDTH;
  const float side_z = c * CAR_HALF_WIDTH;
  const float front_x = c * CAR_HALF_LENGTH;
  const float front_z = -s * CAR_HALF_LENGTH;
  const float roof_side_x = side_x * CAR_ROOF_WIDTH_SCALE;
  const float roof_side_z = side_z * CAR_ROOF_WIDTH_SCALE;
  const float roof_front_x = front_x * CAR_ROOF_FRONT_SCALE;
  const float roof_front_z = front_z * CAR_ROOF_FRONT_SCALE;
  const float roof_rear_x = front_x * CAR_ROOF_REAR_SCALE;
  const float roof_rear_z = front_z * CAR_ROOF_REAR_SCALE;
  Vec3f points[VERTEX_COUNT] = {
    {center_x + front_x - side_x, CAR_BODY_HEIGHT,
     center_z + front_z - side_z},
    {center_x + front_x + side_x, CAR_BODY_HEIGHT,
     center_z + front_z + side_z},
    {center_x - front_x + side_x, CAR_BODY_HEIGHT,
     center_z - front_z + side_z},
    {center_x - front_x - side_x, CAR_BODY_HEIGHT,
     center_z - front_z - side_z},
    {center_x + roof_front_x - roof_side_x, CAR_ROOF_FRONT_HEIGHT,
     center_z + roof_front_z - roof_side_z},
    {center_x + roof_front_x + roof_side_x, CAR_ROOF_FRONT_HEIGHT,
     center_z + roof_front_z + roof_side_z},
    {center_x - roof_rear_x + roof_side_x, CAR_ROOF_REAR_HEIGHT,
     center_z - roof_rear_z + roof_side_z},
    {center_x - roof_rear_x - roof_side_x, CAR_ROOF_REAR_HEIGHT,
     center_z - roof_rear_z - roof_side_z}
  };
  for (int i = 0; i < VERTEX_COUNT; ++i) {
    current_visible_[i] = (uint8_t)project(camera, points[i], current_[i]);
  }
}

void Car::prepare_screen(const Vec2s *points) {
  for (int i = 0; i < VERTEX_COUNT; ++i) {
    current_[i] = points[i];
    current_visible_[i] = 1;
  }
}

ScreenRect Car::previous_bounds(int page) const {
  ScreenRect bounds = {0, 0, 0, 0, 0};
  if (!have_previous_[page]) return bounds;

  for (int i = 0; i < VERTEX_COUNT; ++i) {
    if (!previous_visible_[page][i]) continue;
    if (!bounds.valid) {
      bounds.min_x = previous_[page][i].x;
      bounds.max_x = previous_[page][i].x;
      bounds.min_y = previous_[page][i].y;
      bounds.max_y = previous_[page][i].y;
      bounds.valid = 1;
    } else {
      if (previous_[page][i].x < bounds.min_x) bounds.min_x = previous_[page][i].x;
      if (previous_[page][i].x > bounds.max_x) bounds.max_x = previous_[page][i].x;
      if (previous_[page][i].y < bounds.min_y) bounds.min_y = previous_[page][i].y;
      if (previous_[page][i].y > bounds.max_y) bounds.max_y = previous_[page][i].y;
    }
  }
  if (bounds.valid) {
    bounds.min_x -= 2;
    bounds.min_y -= 2;
    bounds.max_x += 2;
    bounds.max_y += 2;
  }
  return bounds;
}

void Car::draw_wire(const Vec2s *points, const uint8_t *visible,
                    iocs_color_t color) const {
  static const uint8_t edges[EDGE_COUNT][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7},
  };
  for (int i = 0; i < EDGE_COUNT; ++i) {
    const int a = edges[i][0];
    const int b = edges[i][1];
    if (visible[a] && visible[b]) {
      draw_line(points[a].x, points[a].y,
                points[b].x, points[b].y, color);
    }
  }
}

void Car::clear_previous(int page) {
  if (have_previous_[page]) {
    draw_wire(previous_[page], previous_visible_[page], COLOR_BLACK);
  }
}

void Car::render(int page, iocs_color_t color) {
  draw_wire(current_, current_visible_, color);
  for (int i = 0; i < VERTEX_COUNT; ++i) {
    previous_[page][i] = current_[i];
    previous_visible_[page][i] = current_visible_[i];
  }
  have_previous_[page] = 1;
}

int Car::speed() const { return speed_; }

int Car::angle() const { return angle_; }

int Car::offset() const { return offset_; }

int Car::boost() const { return boost_; }

int Car::boosting() const { return boosting_; }

int Car::drift() const { return drift_; }

void Car::apply_impact(int offset_delta, int speed_percent) {
  const int target = offset_ + offset_delta;
  offset_ = target;
  if (offset_ < -LANE_LIMIT) {
    offset_ = -LANE_LIMIT;
    speed_percent -= 15;
  }
  if (offset_ > LANE_LIMIT) {
    offset_ = LANE_LIMIT;
    speed_percent -= 15;
  }
  if (speed_percent < 0) speed_percent = 0;
  speed_ = speed_ * speed_percent / 100;
}

void Car::add_boost(int amount) {
  boost_ += amount;
  if (boost_ > BOOST_LIMIT) boost_ = BOOST_LIMIT;
}

int Car::lap() const { return lap_; }
