#!/usr/bin/env python3

import math

FRAME_COUNT = 61
TRACK_SEGMENTS = 12
TRIG_TABLE_SIZE = 256
INNER_RADIUS = 5.2
OUTER_RADIUS = 8.8
TRACK_RADIUS = 7.0
LANE_SCALE = 0.01875
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
    scale = PROJECTION_SCALE / -transformed[2]
    x = int(FIELD_W * 0.5 + transformed[0] * scale)
    y = int(FIELD_H * 0.5 - transformed[1] * scale)
    if x < 0 or x >= FIELD_W or y < 40 or y >= FIELD_H - 24:
        raise ValueError("intro point is outside the game viewport")
    return (x, y)


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


def make_car_points(offset, trig):
    sine = trig[0]
    cosine = trig[TRIG_TABLE_SIZE // 4]
    radius = TRACK_RADIUS + offset * LANE_SCALE
    center_x = sine * radius
    center_z = cosine * radius
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


def make_frame(frame, trig, car_models):
    t = frame / float(FRAME_COUNT - 1)
    eased = t * t * (3.0 - 2.0 * t)
    height = 20.0 + (11.0 - 20.0) * eased
    radius = 1.5 + (14.0 - 1.5) * eased
    angle = (1.0 - eased) * math.pi * 2.0
    eye = (math.sin(angle) * radius,
           height,
           math.cos(angle) * radius)
    up = (0.0, 1.0, 0.0)
    view = make_view(eye, (0.0, 0.0, 0.0), up)

    track = []
    for radius in (INNER_RADIUS, OUTER_RADIUS):
        ring = []
        for segment in range(TRACK_SEGMENTS):
            index = segment * TRIG_TABLE_SIZE // TRACK_SEGMENTS
            point = (
                trig[index] * radius,
                0.0,
                trig[(index + TRIG_TABLE_SIZE // 4) &
                     (TRIG_TABLE_SIZE - 1)] * radius,
            )
            ring.append(project(view, point))
        track.append(ring)

    cars = [
        [project(view, point) for point in model]
        for model in car_models
    ]
    return track, cars


def points_initializer(points):
    return "{ " + ", ".join("{%d, %d}" % point for point in points) + " }"


def generate():
    trig = make_trig_table()
    car_models = (make_car_points(-42, trig), make_car_points(42, trig))
    frames = [make_frame(frame, trig, car_models)
              for frame in range(FRAME_COUNT)]

    lines = [
        "#ifndef WDR_INTRODAT_H",
        "#define WDR_INTRODAT_H",
        "",
        "#include \"math3d.h\"",
        "",
        "enum {",
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
        "static const IntroFrame kIntroFrames[INTRO_FRAME_COUNT] = {",
    ]
    for track, cars in frames:
        lines.extend((
            "  {",
            "    {",
            "      %s," % points_initializer(track[0]),
            "      %s" % points_initializer(track[1]),
            "    },",
            "    {",
            "      %s," % points_initializer(cars[0]),
            "      %s" % points_initializer(cars[1]),
            "    },",
            "  },",
        ))
    lines.extend((
        "};",
        "",
        "#endif",
        "",
    ))
    return "\n".join(lines)


if __name__ == "__main__":
    print(generate(), end="")
