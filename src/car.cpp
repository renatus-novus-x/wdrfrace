#include "car.h"

namespace {

const int FIELD_W = 512;
const int FIELD_H = 480;
const int ANGLE_LIMIT = 65536;
const int MAX_SPEED = 384;
const int ACCELERATION = 8;
const int BOOST_ACCELERATION = 18;
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
const float CAR_HEIGHT = 0.28f;
const float PROJECTION_SCALE = 300.0f;
const iocs_color_t COLOR_BLACK = 0x0000;

void draw_line(int x0, int y0, int x1, int y1, iocs_color_t color) {
  struct iocs_lineptr line;
  line.x1 = (short)x0;
  line.y1 = (short)y0;
  line.x2 = (short)x1;
  line.y2 = (short)y1;
  line.color = color;
  line.linestyle = 0xffff;
  _iocs_line(&line);
}

}  // namespace

void Car::initialize(int angle, int offset) {
  angle_ = angle;
  offset_ = offset;
  speed_ = 0;
  boost_ = BOOST_LIMIT;
  lap_ = 0;
  have_previous_ = 0;
  for (int i = 0; i < VERTEX_COUNT; ++i) {
    previous_visible_[i] = 0;
    current_visible_[i] = 0;
  }
}

void Car::update(const CarInput &input) {
  if (input.accelerate) speed_ += ACCELERATION;
  if (input.decelerate) speed_ -= DECELERATION;
  if (input.brake) speed_ -= BRAKE_DECELERATION;
  if (input.boost && boost_ > 0) {
    speed_ += BOOST_ACCELERATION;
    boost_ -= BOOST_COST;
    if (boost_ < 0) boost_ = 0;
  }
  if (!input.accelerate && !input.boost && speed_ > 0) {
    speed_ -= COAST_DECELERATION;
  }
  if (speed_ < 0) speed_ = 0;
  if (speed_ > MAX_SPEED) speed_ = MAX_SPEED;

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
  const float center_x = s * radius;
  const float center_z = c * radius;
  const float side_x = s * CAR_HALF_WIDTH;
  const float side_z = c * CAR_HALF_WIDTH;
  const float front_x = c * CAR_HALF_LENGTH;
  const float front_z = -s * CAR_HALF_LENGTH;
  Vec3f points[VERTEX_COUNT] = {
    {center_x + front_x - side_x, CAR_HEIGHT, center_z + front_z - side_z},
    {center_x + front_x + side_x, CAR_HEIGHT, center_z + front_z + side_z},
    {center_x - front_x + side_x, CAR_HEIGHT, center_z - front_z + side_z},
    {center_x - front_x - side_x, CAR_HEIGHT, center_z - front_z - side_z}
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

ScreenRect Car::previous_bounds() const {
  ScreenRect bounds = {0, 0, 0, 0, 0};
  if (!have_previous_) return bounds;

  for (int i = 0; i < VERTEX_COUNT; ++i) {
    if (!previous_visible_[i]) continue;
    if (!bounds.valid) {
      bounds.min_x = previous_[i].x;
      bounds.max_x = previous_[i].x;
      bounds.min_y = previous_[i].y;
      bounds.max_y = previous_[i].y;
      bounds.valid = 1;
    } else {
      if (previous_[i].x < bounds.min_x) bounds.min_x = previous_[i].x;
      if (previous_[i].x > bounds.max_x) bounds.max_x = previous_[i].x;
      if (previous_[i].y < bounds.min_y) bounds.min_y = previous_[i].y;
      if (previous_[i].y > bounds.max_y) bounds.max_y = previous_[i].y;
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
  for (int i = 0; i < VERTEX_COUNT; ++i) {
    const int next = (i + 1) & (VERTEX_COUNT - 1);
    if (visible[i] && visible[next]) {
      draw_line(points[i].x, points[i].y,
                points[next].x, points[next].y, color);
    }
  }
}

void Car::clear_previous() {
  if (have_previous_) {
    draw_wire(previous_, previous_visible_, COLOR_BLACK);
  }
}

void Car::render(iocs_color_t color) {
  draw_wire(current_, current_visible_, color);
  for (int i = 0; i < VERTEX_COUNT; ++i) {
    previous_[i] = current_[i];
    previous_visible_[i] = current_visible_[i];
  }
  have_previous_ = 1;
}

int Car::speed() const { return speed_; }

int Car::lap() const { return lap_; }
