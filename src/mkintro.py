#!/usr/bin/env python3

import math

FRAME_COUNT = 61
COURSE_COUNT = 3
TRACK_SEGMENTS = 12
TRIG_TABLE_SIZE = 256
INNER_RADIUS = 5.2
OUTER_RADIUS = 8.8
TRACK_RADIUS = 7.0
LANE_SCALE = 0.01875
LANE_LIMIT = 64
CAR_HALF_LENGTH = 0.48
CAR_HALF_WIDTH = 0.28
CAR_BODY_HEIGHT = 0.22
CAR_ROOF_FRONT_SCALE = 0.46
CAR_ROOF_REAR_SCALE = 0.60
CAR_ROOF_WIDTH_SCALE = 0.68
CAR_ROOF_FRONT_HEIGHT = 0.62
CAR_ROOF_REAR_HEIGHT = 0.54
PROJECTION_SCALE = 300.0
FIELD_W = 512
FIELD_H = 480
INTRO_VIEW = (16, 72, 496, 452)
RACE_VIEW = (16, 88, 496, 452)
FIT_MARGIN = 4
TRANSITION_FRAMES = 10


def subtract(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def normalize(v):
    length = math.sqrt(dot(v, v))
    return (v[0] / length, v[1] / length, v[2] / length)


def make_view(eye, target, up):
    forward = normalize(subtract(target, eye))
    right = normalize(cross(forward, up))
    camera_up = cross(right, forward)
    backward = (-forward[0], -forward[1], -forward[2])
    return (
        (right[0], right[1], right[2], -dot(right, eye)),
        (camera_up[0], camera_up[1], camera_up[2], -dot(camera_up, eye)),
        (backward[0], backward[1], backward[2], -dot(backward, eye)),
    )


def project(view, point):
    transformed = tuple(
        row[0] * point[0] + row[1] * point[1] +
        row[2] * point[2] + row[3]
        for row in view
    )
    if transformed[2] >= -0.1:
        raise ValueError("intro point is behind the camera")
    scale = PROJECTION_SCALE / -transformed[2]
    return (FIELD_W * 0.5 + transformed[0] * scale,
            FIELD_H * 0.5 - transformed[1] * scale)


def make_trig_table():
    step_sin = 0.024541229
    step_cos = 0.999698819
    result = []
    sine = 0.0
    cosine = 1.0
    for _ in range(TRIG_TABLE_SIZE):
        result.append(sine)
        next_sine = sine * step_cos + cosine * step_sin
        cosine = cosine * step_cos - sine * step_sin
        sine = next_sine
    return result


def course_point(course, sine, cosine, radius):
    x = sine * radius
    z = cosine * radius
    if course == 1:
        return (x * 1.12, z * 0.82)
    if course == 2:
        cosine3 = 4.0 * cosine * cosine * cosine - 3.0 * cosine
        scale = 1.0 + 0.11 * cosine3
        return (x * scale, z * scale)
    return (x, z)


def make_car_points(course, offset, trig, index=0):
    sine = trig[index]
    cosine = trig[(index + TRIG_TABLE_SIZE // 4) &
                  (TRIG_TABLE_SIZE - 1)]
    radius = TRACK_RADIUS + offset * LANE_SCALE
    center_x, center_z = course_point(course, sine, cosine, radius)
    side_x = sine * CAR_HALF_WIDTH
    side_z = cosine * CAR_HALF_WIDTH
    front_x = cosine * CAR_HALF_LENGTH
    front_z = -sine * CAR_HALF_LENGTH
    roof_side_x = side_x * CAR_ROOF_WIDTH_SCALE
    roof_side_z = side_z * CAR_ROOF_WIDTH_SCALE
    roof_front_x = front_x * CAR_ROOF_FRONT_SCALE
    roof_front_z = front_z * CAR_ROOF_FRONT_SCALE
    roof_rear_x = front_x * CAR_ROOF_REAR_SCALE
    roof_rear_z = front_z * CAR_ROOF_REAR_SCALE
    return (
        (center_x + front_x - side_x, CAR_BODY_HEIGHT,
         center_z + front_z - side_z),
        (center_x + front_x + side_x, CAR_BODY_HEIGHT,
         center_z + front_z + side_z),
        (center_x - front_x + side_x, CAR_BODY_HEIGHT,
         center_z - front_z + side_z),
        (center_x - front_x - side_x, CAR_BODY_HEIGHT,
         center_z - front_z - side_z),
        (center_x + roof_front_x - roof_side_x, CAR_ROOF_FRONT_HEIGHT,
         center_z + roof_front_z - roof_side_z),
        (center_x + roof_front_x + roof_side_x, CAR_ROOF_FRONT_HEIGHT,
         center_z + roof_front_z + roof_side_z),
        (center_x - roof_rear_x + roof_side_x, CAR_ROOF_REAR_HEIGHT,
         center_z - roof_rear_z + roof_side_z),
        (center_x - roof_rear_x - roof_side_x, CAR_ROOF_REAR_HEIGHT,
         center_z - roof_rear_z - roof_side_z),
    )


def frame_view(frame):
    t = frame / float(FRAME_COUNT - 1)
    eased = t * t * (3.0 - 2.0 * t)
    height = 20.0 + (11.0 - 20.0) * eased
    radius = 1.5 + (14.0 - 1.5) * eased
    angle = (1.0 - eased) * math.pi * 2.0
    eye = (math.sin(angle) * radius,
           height,
           math.cos(angle) * radius)
    return make_view(eye, (0.0, 0.0, 0.0), (0.0, 1.0, 0.0))


def make_frame(course, frame, trig, car_models):
    view = frame_view(frame)
    track = []
    for radius in (INNER_RADIUS, OUTER_RADIUS):
        ring = []
        for segment in range(TRACK_SEGMENTS):
            index = segment * TRIG_TABLE_SIZE // TRACK_SEGMENTS
            sine = trig[index]
            cosine = trig[(index + TRIG_TABLE_SIZE // 4) &
                          (TRIG_TABLE_SIZE - 1)]
            x, z = course_point(course, sine, cosine, radius)
            ring.append(project(view, (x, 0.0, z)))
        track.append(ring)
    cars = [[project(view, point) for point in model]
            for model in car_models]
    return track, cars


def frame_points(frame):
    track, cars = frame
    return [point for group in track + cars for point in group]


def fit_affine(points, viewport):
    source_left = min(point[0] for point in points)
    source_right = max(point[0] for point in points)
    source_top = min(point[1] for point in points)
    source_bottom = max(point[1] for point in points)
    target_left = viewport[0] + FIT_MARGIN
    target_top = viewport[1] + FIT_MARGIN
    target_right = viewport[2] - FIT_MARGIN
    target_bottom = viewport[3] - FIT_MARGIN
    scale = min((target_right - target_left) /
                (source_right - source_left),
                (target_bottom - target_top) /
                (source_bottom - source_top))
    source_cx = (source_left + source_right) * 0.5
    source_cy = (source_top + source_bottom) * 0.5
    target_cx = (target_left + target_right) * 0.5
    target_cy = (target_top + target_bottom) * 0.5
    transform = (scale,
                 target_cx - source_cx * scale,
                 target_cy - source_cy * scale)
    return transform, (source_left, source_top,
                       source_right, source_bottom)


def blend_affine(first, second, amount):
    return tuple(first[i] + (second[i] - first[i]) * amount
                 for i in range(3))


def transform_point(point, transform):
    scale, translate_x, translate_y = transform
    return (round(point[0] * scale + translate_x),
            round(point[1] * scale + translate_y))


def transform_frame(frame, transform):
    track, cars = frame
    return ([[transform_point(point, transform) for point in ring]
             for ring in track],
            [[transform_point(point, transform) for point in car]
             for car in cars])


def validate_frame(frame):
    for x, y in frame_points(frame):
        if x < 0 or x >= FIELD_W or y < 40 or y >= FIELD_H - 24:
            raise ValueError("fitted point is outside the game viewport")


def race_fit(course, trig, final_frame):
    view = frame_view(FRAME_COUNT - 1)
    points = frame_points(final_frame)
    for index in range(TRIG_TABLE_SIZE):
        for offset in (-LANE_LIMIT, LANE_LIMIT):
            points.extend(project(view, point) for point in
                          make_car_points(course, offset, trig, index))
    return fit_affine(points, RACE_VIEW)


def points_initializer(points):
    return "{ " + ", ".join("{%d, %d}" % point for point in points) + " }"


def generate():
    trig = make_trig_table()
    courses = []
    race_tracks = []
    race_projections = []
    fit_comments = []
    transition_start = FRAME_COUNT - TRANSITION_FRAMES

    for course in range(COURSE_COUNT):
        car_models = (make_car_points(course, -42, trig),
                      make_car_points(course, 42, trig))
        raw_frames = [make_frame(course, frame, trig, car_models)
                      for frame in range(FRAME_COUNT)]
        intro_points = [point
                        for frame in raw_frames[:transition_start + 1]
                        for point in frame_points(frame)]
        intro_transform, intro_bounds = fit_affine(intro_points, INTRO_VIEW)
        race_transform, race_bounds = race_fit(course, trig, raw_frames[-1])

        frames = []
        for frame, raw_frame in enumerate(raw_frames):
            transform = intro_transform
            if frame > transition_start:
                amount = ((frame - transition_start) /
                          float(FRAME_COUNT - 1 - transition_start))
                amount = amount * amount * (3.0 - 2.0 * amount)
                transform = blend_affine(intro_transform,
                                         race_transform, amount)
            fitted = transform_frame(raw_frame, transform)
            validate_frame(fitted)
            frames.append(fitted)
        courses.append(frames)

        race_frame = transform_frame(raw_frames[-1], race_transform)
        validate_frame(race_frame)
        race_tracks.append(race_frame[0])
        race_scale, race_tx, race_ty = race_transform
        race_projections.append((
            PROJECTION_SCALE * race_scale,
            FIELD_W * 0.5 * race_scale + race_tx,
            FIELD_H * 0.5 * race_scale + race_ty,
        ))
        fit_comments.append((intro_bounds, intro_transform[0],
                             race_bounds, race_transform[0]))

    lines = [
        "#ifndef WDR_INTRODAT_H",
        "#define WDR_INTRODAT_H",
        "",
        "#include \"math3d.h\"",
        "",
        "enum {",
        "  INTRO_COURSE_COUNT = %d," % COURSE_COUNT,
        "  INTRO_FRAME_COUNT = %d," % FRAME_COUNT,
        "  INTRO_TRACK_SEGMENTS = %d," % TRACK_SEGMENTS,
        "  INTRO_CAR_VERTICES = 8,",
        "};",
        "",
        "struct IntroFrame {",
        "  Vec2s track[2][INTRO_TRACK_SEGMENTS];",
        "  Vec2s cars[2][INTRO_CAR_VERTICES];",
        "};",
        "",
        "struct RaceProjection {",
        "  float scale;",
        "  float center_x;",
        "  float center_y;",
        "};",
        "",
    ]
    for course, info in enumerate(fit_comments):
        lines.append("// Course %d intro bbox %s scale %.6f; "
                     "race bbox %s scale %.6f." %
                     (course, info[0], info[1], info[2], info[3]))
    lines.extend((
        "static const RaceProjection "
        "kRaceProjections[INTRO_COURSE_COUNT] = {",
    ))
    for scale, center_x, center_y in race_projections:
        lines.append("  {%.6ff, %.6ff, %.6ff}," %
                     (scale, center_x, center_y))
    lines.extend((
        "};",
        "",
        "static const Vec2s "
        "kRaceTracks[INTRO_COURSE_COUNT][2][INTRO_TRACK_SEGMENTS] = {",
    ))
    for track in race_tracks:
        lines.extend((
            "  {",
            "    %s," % points_initializer(track[0]),
            "    %s" % points_initializer(track[1]),
            "  },",
        ))
    lines.extend((
        "};",
        "",
        "static const IntroFrame "
        "kIntroFrames[INTRO_COURSE_COUNT][INTRO_FRAME_COUNT] = {",
    ))
    for frames in courses:
        lines.append("  {")
        for track, cars in frames:
            lines.extend((
                "    {",
                "      {",
                "        %s," % points_initializer(track[0]),
                "        %s" % points_initializer(track[1]),
                "      },",
                "      {",
                "        %s," % points_initializer(cars[0]),
                "        %s" % points_initializer(cars[1]),
                "      },",
                "    },",
            ))
        lines.append("  },")
    lines.extend((
        "};",
        "",
        "#endif",
        "",
    ))
    return "\n".join(lines)


if __name__ == "__main__":
    print(generate(), end="")
