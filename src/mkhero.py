#!/usr/bin/env python3
import math

OUT = "herodat.h"
HIDDEN = -32768
XMIN, XMAX = 16.0, 495.0
YMIN, YMAX = 124.0, 393.0
CX, CY = 255.5, 258.5
SCALE = 230.0
FRAMES = 160
SHOT_LENGTH = 40
HERO_LEFT, HERO_RIGHT = 28, 484
HERO_TOP, HERO_BOTTOM = 140, 374
HERO_MARGIN = 4


def sub(a, b):
    return tuple(a[i] - b[i] for i in range(3))


def dot(a, b):
    return sum(a[i] * b[i] for i in range(3))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def normalize(v):
    length = math.sqrt(dot(v, v))
    return tuple(x / length for x in v)


def camera(eye, target):
    forward = normalize(sub(target, eye))
    right = normalize(cross(forward, (0.0, 1.0, 0.0)))
    up = cross(right, forward)
    return eye, right, up, forward


def project(point, view):
    eye, right, up, forward = view
    relative = sub(point, eye)
    x = dot(relative, right)
    y = dot(relative, up)
    z = dot(relative, forward)
    if z <= 0.1:
        return None
    return (CX + SCALE * x / z, CY - SCALE * y / z)


def clip_line(a, b):
    if a is None or b is None:
        return None
    x0, y0 = a
    x1, y1 = b
    dx, dy = x1 - x0, y1 - y0
    p = (-dx, dx, -dy, dy)
    q = (x0 - XMIN, XMAX - x0, y0 - YMIN, YMAX - y0)
    u0, u1 = 0.0, 1.0
    for pi, qi in zip(p, q):
        if abs(pi) < 1.0e-9:
            if qi < 0.0:
                return None
            continue
        t = qi / pi
        if pi < 0.0:
            u0 = max(u0, t)
        else:
            u1 = min(u1, t)
        if u0 > u1:
            return None
    return ((round(x0 + u0 * dx), round(y0 + u0 * dy)),
            (round(x0 + u1 * dx), round(y0 + u1 * dy)))


def line(a, b, view):
    clipped = clip_line(project(a, view), project(b, view))
    if clipped is None:
        return ((HIDDEN, 0), (HIDDEN, 0))
    return clipped


def car_lines(cx, cz, heading, view):
    local = [
        (-0.50, 0.22, 1.00), (0.50, 0.22, 1.00),
        (0.50, 0.22, -1.00), (-0.50, 0.22, -1.00),
        (-0.34, 0.82, 0.45), (0.34, 0.82, 0.45),
        (0.34, 0.72, -0.60), (-0.34, 0.72, -0.60),
    ]
    edges = [(0, 1), (1, 2), (2, 3), (3, 0),
             (4, 5), (5, 6), (6, 7), (7, 4),
             (0, 4), (1, 5), (2, 6), (3, 7)]
    cs, sn = math.cos(heading), math.sin(heading)
    world = []
    for x, y, z in local:
        world.append((cx + x * cs + z * sn,
                      y,
                      cz - x * sn + z * cs))
    return [line(world[a], world[b], view) for a, b in edges]


def format_line(segment):
    return "{{%d, %d}, {%d, %d}}" % (
        segment[0][0], segment[0][1], segment[1][0], segment[1][1])


def fit_shot(shot):
    floor, cars, lights = shot
    groups = floor + cars + [lights]
    points = [point for group in groups for segment in group
              if segment[0][0] != HIDDEN for point in segment]
    if not points:
        raise ValueError("hero shot has no visible geometry")

    source_left = min(point[0] for point in points)
    source_right = max(point[0] for point in points)
    source_top = min(point[1] for point in points)
    source_bottom = max(point[1] for point in points)
    target_left = HERO_LEFT + HERO_MARGIN
    target_right = HERO_RIGHT - HERO_MARGIN
    target_top = HERO_TOP + HERO_MARGIN
    target_bottom = HERO_BOTTOM - HERO_MARGIN
    scale = min((target_right - target_left) /
                float(source_right - source_left),
                (target_bottom - target_top) /
                float(source_bottom - source_top))
    source_cx = (source_left + source_right) * 0.5
    source_cy = (source_top + source_bottom) * 0.5
    target_cx = (target_left + target_right) * 0.5
    target_cy = (target_top + target_bottom) * 0.5

    def transform_group(group):
        transformed = []
        for segment in group:
            if segment[0][0] == HIDDEN:
                transformed.append(segment)
                continue
            transformed.append(tuple(
                (round(target_cx + (point[0] - source_cx) * scale),
                 round(target_cy + (point[1] - source_cy) * scale))
                for point in segment))
        return transformed

    fitted = ([transform_group(group) for group in floor],
              [transform_group(group) for group in cars],
              transform_group(lights))
    fitted_points = [point for group in fitted[0] + fitted[1] + [fitted[2]]
                     for segment in group if segment[0][0] != HIDDEN
                     for point in segment]
    fitted_bounds = (min(point[0] for point in fitted_points),
                     min(point[1] for point in fitted_points),
                     max(point[0] for point in fitted_points),
                     max(point[1] for point in fitted_points))
    if (fitted_bounds[0] < HERO_LEFT or fitted_bounds[2] > HERO_RIGHT or
            fitted_bounds[1] < HERO_TOP or fitted_bounds[3] > HERO_BOTTOM):
        raise ValueError("fitted hero shot exceeds its viewport")
    source_bounds = (source_left, source_top, source_right, source_bottom)
    return fitted, (source_bounds, scale, fitted_bounds)


shots = []
cameras = [
    ((8.5, 4.2, 10.0), (0.0, 0.35, 0.0)),
    ((-9.0, 3.2, 6.5), (0.0, 0.35, 0.0)),
    ((0.0, 9.5, 7.5), (0.0, 0.35, 0.0)),
    ((8.0, 2.8, -8.0), (0.0, 0.35, 0.0)),
]

for eye, target in cameras:
    view = camera(eye, target)
    floor = [[], []]
    for i in range(8):
        x = -6.0 + i * (12.0 / 7.0)
        floor[0].append(line((x, 0.0, -5.0), (x, 0.0, 6.0), view))
        z = -5.0 + i * (11.0 / 7.0)
        floor[1].append(line((-6.0, 0.0, z), (6.0, 0.0, z), view))
    cars = [car_lines(-1.45, 0.35, 0.08, view),
            car_lines(1.45, -0.35, -0.08, view)]
    lights = []
    for i in range(8):
        z = -4.0 + i * (8.0 / 7.0)
        lights.append(line((-0.42, 0.03, z), (0.42, 0.03, z), view))
    shots.append((floor, cars, lights))

fit_info = []
for index, shot in enumerate(shots):
    shots[index], info = fit_shot(shot)
    fit_info.append(info)

with open(OUT, "w", newline="\n") as out:
    out.write("#ifndef WDRFRACE_HERODAT_H\n")
    out.write("#define WDRFRACE_HERODAT_H\n\n")
    out.write("enum { HERO_FRAME_COUNT = 160, HERO_SHOT_COUNT = 4, ")
    out.write("HERO_FLOOR_EDGES = 8, HERO_CAR_EDGES = 12, ")
    out.write("HERO_LIGHT_COUNT = 8, HERO_PULSE_COUNT = 8 };\n")
    out.write("enum { HERO_FLAG_CUT = 1 };\n\n")
    out.write("struct HeroShot {\n")
    out.write("  Vec2s floor[2][HERO_FLOOR_EDGES][2];\n")
    out.write("  Vec2s cars[2][HERO_CAR_EDGES][2];\n")
    out.write("  Vec2s lights[HERO_LIGHT_COUNT][2];\n")
    out.write("};\n\n")
    out.write("struct HeroFrame {\n")
    out.write("  unsigned char shot;\n")
    out.write("  unsigned char flags;\n")
    out.write("  unsigned char edge_phase;\n")
    out.write("  unsigned char light_phase;\n")
    out.write("  unsigned char pulse_phase;\n")
    out.write("};\n\n")
    out.write("// Per-shot screen-space fit includes floor, lights, and both cars.\n")
    for index, (source, scale, fitted) in enumerate(fit_info):
        out.write("// Shot %d: source %s, scale %.4f, fitted %s.\n" %
                  (index, source, scale, fitted))
    out.write("static const HeroShot kHeroShots[HERO_SHOT_COUNT] = {\n")
    for floor, cars, lights in shots:
        out.write("  {\n    {\n")
        for group in floor:
            out.write("      {%s},\n" % ", ".join(format_line(x) for x in group))
        out.write("    },\n    {\n")
        for car in cars:
            out.write("      {%s},\n" % ", ".join(format_line(x) for x in car))
        out.write("    },\n")
        out.write("    {%s}\n" % ", ".join(format_line(x) for x in lights))
        out.write("  },\n")
    out.write("};\n\n")
    out.write("static const HeroFrame kHeroFrames[HERO_FRAME_COUNT] = {\n")
    for frame in range(FRAMES):
        local = frame % SHOT_LENGTH
        values = (frame // SHOT_LENGTH,
                  1 if local == 0 else 0,
                  (frame // 2) % 12,
                  (frame // 3) % 8,
                  (frame // 4) % 8)
        out.write("  {%d, %d, %d, %d, %d},\n" % values)
    out.write("};\n\n#endif\n")
