#include "gmcourse.h"

#include "coursdat.h"
#include "screen.h"
#include "vtext.h"

namespace {

const iocs_color_t COLOR_BLACK = 0x0000;
const iocs_color_t COLOR_WHITE = 0xffff;
const iocs_color_t COLOR_CYAN = 0xf83f;
const iocs_color_t COLOR_TRACK = 0x7bef;
const iocs_color_t COLOR_DIM = 0x2109;
const iocs_color_t COLOR_GATE = 0xffe0;

const char *kCourseNames[COURSE_PREVIEW_COUNT] = {
  "RING", "OVAL", "PULSE"
};

const char *kCourseDescriptions[COURSE_PREVIEW_COUNT] = {
  "BALANCED CLASSIC LOOP",
  "WIDE HIGH SPEED LOOP",
  "THREE APEX TECHNICAL LOOP"
};

}  // namespace

GameModeCourse::GameModeCourse()
    : selected_course_(0),
      frame_(0),
      input_released_(0),
      direction_down_(0),
      select_sound_pending_(0) {
  for (int page = 0; page < 2; ++page) {
    drawn_course_[page] = -1;
    drawn_frame_[page] = -1;
  }
}

int GameModeCourse::course_id() const { return selected_course_; }

int GameModeCourse::initialize() {
  input_.update();
  input_released_ = !input_.confirm();
  direction_down_ = input_.menu_left() || input_.menu_right();
  select_sound_pending_ = 0;
  frame_ = 0;
  for (int page = 0; page < 2; ++page) {
    drawn_course_[page] = -1;
    drawn_frame_[page] = -1;
  }
  return 1;
}

GameModeId GameModeCourse::update() {
  input_.update();
  if (input_.cancel()) return GAME_MODE_TITLE;

  const int direction = input_.menu_left() || input_.menu_right();
  if (direction && !direction_down_) {
    if (input_.menu_left()) {
      selected_course_ = (selected_course_ + COURSE_PREVIEW_COUNT - 1) %
                         COURSE_PREVIEW_COUNT;
    } else {
      selected_course_ = (selected_course_ + 1) % COURSE_PREVIEW_COUNT;
    }
    select_sound_pending_ = 1;
  }
  direction_down_ = direction;

  if (!input_released_) {
    if (!input_.confirm()) input_released_ = 1;
  } else if (input_.confirm()) {
    return GAME_MODE_HOW_TO_PLAY;
  }

  frame_ = (frame_ + 1) % COURSE_PREVIEW_FRAME_COUNT;
  return GAME_MODE_COURSE_SELECT;
}

int GameModeCourse::consume_select_sound() {
  const int pending = select_sound_pending_;
  select_sound_pending_ = 0;
  return pending;
}

void GameModeCourse::draw_scene() const {
  screen_clear(COLOR_BLACK);
  screen_text_tracking(24, 8, "TRACK CONFIG", 1, 1, COLOR_CYAN);
  screen_text(466, 8, "02", 1, COLOR_WHITE);
  screen_line(24, 22, 488, 22, COLOR_TRACK);
  vector_centered("SELECT COURSE", 34, 6, 2, 2, COLOR_WHITE);
  screen_line(156, 84, 356, 84, COLOR_CYAN);

  screen_line(72, 368, 56, 384, COLOR_CYAN);
  screen_line(56, 384, 72, 400, COLOR_CYAN);
  screen_line(440, 368, 456, 384, COLOR_CYAN);
  screen_line(456, 384, 440, 400, COLOR_CYAN);
  screen_centered_tracking("LEFT RIGHT SELECT", 420, 1, 3, COLOR_WHITE);
  screen_centered_tracking("SPACE CONFIRM   ESC TITLE", 448,
                           1, 2, COLOR_WHITE);
}

void GameModeCourse::draw_ring(const Vec2s *ring,
                               iocs_color_t color) const {
  for (int i = 0; i < COURSE_PREVIEW_SEGMENTS; ++i) {
    const int next = (i + 1) % COURSE_PREVIEW_SEGMENTS;
    screen_line(ring[i].x, ring[i].y, ring[next].x, ring[next].y, color);
  }
}

void GameModeCourse::draw_marker(
    const Vec2s track[2][12], int segment, int low, int high,
    iocs_color_t color) const {
  const Vec2s &inner = track[0][segment];
  const Vec2s &outer = track[1][segment];
  const int x0 = inner.x + (outer.x - inner.x) * low / 12;
  const int y0 = inner.y + (outer.y - inner.y) * low / 12;
  const int x1 = inner.x + (outer.x - inner.x) * high / 12;
  const int y1 = inner.y + (outer.y - inner.y) * high / 12;
  screen_line(x0, y0, x1, y1, color);
}

void GameModeCourse::draw_course(int course, int frame,
                                 iocs_color_t color) const {
  const CoursePreviewFrame &preview = kCoursePreview[course][frame];
  draw_ring(preview.track[0], color);
  draw_ring(preview.track[1], color);
  draw_marker(preview.track, 0, 0, 12,
              color == COLOR_BLACK ? COLOR_BLACK : COLOR_CYAN);
  for (int segment = 3; segment < COURSE_PREVIEW_SEGMENTS; segment += 3) {
    draw_marker(preview.track, segment, 4, 8,
                color == COLOR_BLACK ? COLOR_BLACK : COLOR_GATE);
  }
}

void GameModeCourse::draw_course_label(int course,
                                       iocs_color_t color) const {
  vector_centered(kCourseNames[course], 354, 5, 3, 1, color);
  screen_centered_tracking(kCourseDescriptions[course], 398,
                           1, 2, color == COLOR_BLACK ?
                           COLOR_BLACK : COLOR_DIM);
}

void GameModeCourse::render(int page) {
  if (drawn_frame_[page] < 0) {
    draw_scene();
    draw_course(selected_course_, frame_, COLOR_TRACK);
    draw_course_label(selected_course_, COLOR_CYAN);
  } else {
    draw_course(drawn_course_[page], drawn_frame_[page], COLOR_BLACK);
    if (drawn_course_[page] != selected_course_) {
      draw_course_label(drawn_course_[page], COLOR_BLACK);
      draw_course_label(selected_course_, COLOR_CYAN);
    }
    draw_course(selected_course_, frame_, COLOR_TRACK);
  }
  drawn_course_[page] = selected_course_;
  drawn_frame_[page] = frame_;
}

void GameModeCourse::finalize() {}
