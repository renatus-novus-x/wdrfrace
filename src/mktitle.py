#!/usr/bin/env python3

import math

FRAME_COUNT = 160
SHOT_LENGTH = 32
TRACK_SEGMENTS = 8
FIELD_W = 512
VIEW_CENTER_Y = 278
PROJECTION_SCALE = 170.0
REPLAY_LEFT = 28
REPLAY_TOP = 104
REPLAY_RIGHT = 484
REPLAY_BOTTOM = 410
FIT_MARGIN = 4
INNER_RADIUS = 5.2
OUTER_RADIUS = 8.8
TRACK_RADIUS = 7.0
CAR_HALF_LENGTH = 0.55
CAR_HALF_WIDTH = 0.32
CAR_BODY_HEIGHT = 0.22
CAR_ROOF_FRONT_SCALE = 0.46
CAR_ROOF_REAR_SCALE = 0.60
CAR_ROOF_WIDTH_SCALE = 0.68
CAR_ROOF_FRONT_HEIGHT = 0.62
CAR_ROOF_REAR_HEIGHT = 0.54
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
    return (
        FIELD_W * 0.5 + transformed[0] * scale,
        VIEW_CENTER_Y - transformed[1] * scale,
    )


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


def frame_points(frame):
    track, cars, _ = frame
    return [point for ring in track for point in ring] + [
        point for car in cars for point in car
    ]


def bounds_for_frames(frames):
    points = [point for frame in frames for point in frame_points(frame)]
    return (
        min(point[0] for point in points),
        min(point[1] for point in points),
        max(point[0] for point in points),
        max(point[1] for point in points),
    )


def fit_point(point, source_bounds, scale):
    left, top, right, bottom = source_bounds
    source_center_x = (left + right) * 0.5
    source_center_y = (top + bottom) * 0.5
    target_center_x = (REPLAY_LEFT + REPLAY_RIGHT) * 0.5
    target_center_y = (REPLAY_TOP + REPLAY_BOTTOM) * 0.5
    return (
        int(round(target_center_x + (point[0] - source_center_x) * scale)),
        int(round(target_center_y + (point[1] - source_center_y) * scale)),
    )


def fit_shots(frames):
    fitted = list(frames)
    shot_info = []
    shot_count = (FRAME_COUNT + SHOT_LENGTH - 1) // SHOT_LENGTH
    target_width = REPLAY_RIGHT - REPLAY_LEFT - FIT_MARGIN * 2
    target_height = REPLAY_BOTTOM - REPLAY_TOP - FIT_MARGIN * 2
    for shot in range(shot_count):
        first = shot * SHOT_LENGTH
        last = min(first + SHOT_LENGTH, FRAME_COUNT)
        source_bounds = bounds_for_frames(frames[first:last])
        width = source_bounds[2] - source_bounds[0]
        height = source_bounds[3] - source_bounds[1]
        scale = min(target_width / width, target_height / height)
        for frame in range(first, last):
            track, cars, flags = frames[frame]
            fitted_track = [
                [fit_point(point, source_bounds, scale) for point in ring]
                for ring in track
            ]
            fitted_cars = [
                [fit_point(point, source_bounds, scale) for point in car]
                for car in cars
            ]
            fitted[frame] = (fitted_track, fitted_cars, flags)
        fitted_bounds = bounds_for_frames(fitted[first:last])
        if (fitted_bounds[0] < REPLAY_LEFT or
                fitted_bounds[1] < REPLAY_TOP or
                fitted_bounds[2] > REPLAY_RIGHT or
                fitted_bounds[3] > REPLAY_BOTTOM):
            raise ValueError("fitted replay shot is outside the viewport")
        shot_info.append((source_bounds, fitted_bounds, scale))
    return fitted, shot_info


def points_initializer(points):
    return "{ " + ", ".join("{%d, %d}" % point for point in points) + " }"


def generate():
    raw_frames = [make_frame(frame) for frame in range(FRAME_COUNT)]
    frames, shot_info = fit_shots(raw_frames)
    lines = [
        "#ifndef WDR_DEMODAT_H",
        "#define WDR_DEMODAT_H",
        "",
        "#include \"math3d.h\"",
        "",
        "enum {",
        "  DEMO_FRAME_COUNT = %d," % FRAME_COUNT,
        "  DEMO_SHOT_LENGTH = %d," % SHOT_LENGTH,
        "  DEMO_SHOT_COUNT = %d," % len(shot_info),
        "  DEMO_TRACK_SEGMENTS = %d," % TRACK_SEGMENTS,
        "  DEMO_CAR_VERTICES = 8,",
        "  DEMO_CAR_EDGES = 12,",
        "  DEMO_FLAG_CUT = %d," % FLAG_CUT,
        "  DEMO_FLAG_TRACK_MOVED = %d," % FLAG_TRACK_MOVED,
        "};",
        "",
        "struct DemoFrame {",
        "  Vec2s track[2][DEMO_TRACK_SEGMENTS];",
        "  Vec2s cars[2][DEMO_CAR_VERTICES];",
        "  unsigned char flags;",
        "};",
        "",
        "static const DemoFrame kDemoFrames[DEMO_FRAME_COUNT] = {",
    ]
    for frame_index, (track, cars, flags) in enumerate(frames):
        if frame_index % SHOT_LENGTH == 0:
            shot = frame_index // SHOT_LENGTH
            source, fitted, scale = shot_info[shot]
            lines.append(
                "  /* shot %d: source %.1f,%.1f-%.1f,%.1f; "
                "scale %.3f; fitted %d,%d-%d,%d */" % (
                    shot, source[0], source[1], source[2], source[3],
                    scale, fitted[0], fitted[1], fitted[2], fitted[3]))
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
