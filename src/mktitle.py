#!/usr/bin/env python3

import math

FRAME_COUNT = 160
SHOT_LENGTH = 32
TRACK_SEGMENTS = 8
FIELD_W = 512
VIEW_CENTER_Y = 278
PROJECTION_SCALE = 170.0
INNER_RADIUS = 5.2
OUTER_RADIUS = 8.8
TRACK_RADIUS = 7.0
CAR_HALF_LENGTH = 0.55
CAR_HALF_WIDTH = 0.32
CAR_HEIGHT = 0.3
FLAG_CUT = 1
FLAG_TRACK_MOVED = 2


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


def make_view(eye, target):
    forward = normalize(subtract(target, eye))
    right = normalize(cross(forward, (0.0, 1.0, 0.0)))
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
    if transformed[2] >= -1.0:
        raise ValueError("title point is behind the near plane")
    scale = PROJECTION_SCALE / -transformed[2]
    x = int(FIELD_W * 0.5 + transformed[0] * scale)
    y = int(VIEW_CENTER_Y - transformed[1] * scale)
    if x < 24 or x > 487 or y < 122 or y > 409:
        raise ValueError("title point is outside the replay viewport")
    return (x, y)


def smooth(value):
    return value * value * (3.0 - 2.0 * value)


def camera_for_frame(frame, average_angle):
    shot = frame // SHOT_LENGTH
    local = (frame % SHOT_LENGTH) / float(SHOT_LENGTH - 1)
    eased = smooth(local)
    if shot == 0:
        angle = 0.15 + eased * 1.35
        radius = 18.0 - eased * 2.0
        return ((math.sin(angle) * radius, 14.0 - eased * 5.0,
                 math.cos(angle) * radius), (0.0, 0.0, 0.0), True)
    if shot == 1:
        return ((13.0, 4.0, 11.0), (0.0, 0.0, 0.0), False)
    if shot == 2:
        angle = average_angle - 0.8 + eased * 0.7
        radius = 15.0
        return ((math.sin(angle) * radius, 5.5,
                 math.cos(angle) * radius), (0.0, 0.0, 0.0), True)
    if shot == 3:
        return ((-2.0, 3.5, 17.0), (0.0, 0.0, 0.0), False)
    angle = 3.7 - eased * 1.2
    radius = 16.0
    return ((math.sin(angle) * radius, 10.0 + eased * 5.0,
             math.cos(angle) * radius), (0.0, 0.0, 0.0), True)


def make_car_points(angle, lane):
    sine = math.sin(angle)
    cosine = math.cos(angle)
    radius = TRACK_RADIUS + lane
    center_x = sine * radius
    center_z = cosine * radius
    side_x = sine * CAR_HALF_WIDTH
    side_z = cosine * CAR_HALF_WIDTH
    front_x = cosine * CAR_HALF_LENGTH
    front_z = -sine * CAR_HALF_LENGTH
    return (
        (center_x + front_x - side_x, CAR_HEIGHT,
         center_z + front_z - side_z),
        (center_x + front_x + side_x, CAR_HEIGHT,
         center_z + front_z + side_z),
        (center_x - front_x + side_x, CAR_HEIGHT,
         center_z - front_z + side_z),
        (center_x - front_x - side_x, CAR_HEIGHT,
         center_z - front_z - side_z),
    )


def make_frame(frame):
    progress = frame / float(FRAME_COUNT)
    base_angle = progress * math.pi * 2.0 * 1.45
    lead = 0.42 * math.sin(progress * math.pi * 4.0)
    angle_p1 = base_angle + lead * 0.5
    angle_p2 = base_angle - lead * 0.5
    lane_wave = 0.72 * math.sin(progress * math.pi * 4.0 + 0.7)
    lanes = (-lane_wave, lane_wave)

    eye, target, moving = camera_for_frame(
        frame, (angle_p1 + angle_p2) * 0.5)
    view = make_view(eye, target)
    track = []
    for radius in (INNER_RADIUS, OUTER_RADIUS):
        ring = []
        for segment in range(TRACK_SEGMENTS):
            angle = segment * math.pi * 2.0 / TRACK_SEGMENTS
            ring.append(project(view, (
                math.sin(angle) * radius,
                0.0,
                math.cos(angle) * radius,
            )))
        track.append(ring)

    car_models = (
        make_car_points(angle_p1, lanes[0]),
        make_car_points(angle_p2, lanes[1]),
    )
    cars = [[project(view, point) for point in model]
            for model in car_models]
    flags = FLAG_TRACK_MOVED if moving else 0
    if frame % SHOT_LENGTH == 0:
        flags |= FLAG_CUT
    return track, cars, flags


def points_initializer(points):
    return "{ " + ", ".join("{%d, %d}" % point for point in points) + " }"


def generate():
    frames = [make_frame(frame) for frame in range(FRAME_COUNT)]
    lines = [
        "#ifndef WDR_DEMODAT_H",
        "#define WDR_DEMODAT_H",
        "",
        "#include \"math3d.h\"",
        "",
        "enum {",
        "  DEMO_FRAME_COUNT = %d," % FRAME_COUNT,
        "  DEMO_TRACK_SEGMENTS = %d," % TRACK_SEGMENTS,
        "  DEMO_FLAG_CUT = %d," % FLAG_CUT,
        "  DEMO_FLAG_TRACK_MOVED = %d," % FLAG_TRACK_MOVED,
        "};",
        "",
        "struct DemoFrame {",
        "  Vec2s track[2][DEMO_TRACK_SEGMENTS];",
        "  Vec2s cars[2][4];",
        "  unsigned char flags;",
        "};",
        "",
        "static const DemoFrame kDemoFrames[DEMO_FRAME_COUNT] = {",
    ]
    for track, cars, flags in frames:
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
            "    %d" % flags,
            "  },",
        ))
    lines.extend(("};", "", "#endif", ""))
    return "\n".join(lines)


if __name__ == "__main__":
    print(generate(), end="")
