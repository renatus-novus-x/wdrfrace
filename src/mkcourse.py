#!/usr/bin/env python3

import math

COURSE_COUNT = 3
FRAME_COUNT = 40
SEGMENTS = 12
INNER_RADIUS = 5.2
OUTER_RADIUS = 8.8
FIELD_W = 512
PREVIEW_CENTER_Y = 168
PROJECTION_SCALE = 300.0


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
    return (
        int(FIELD_W * 0.5 + transformed[0] * scale),
        int(PREVIEW_CENTER_Y - transformed[1] * scale),
    )


def course_point(course, angle, radius):
    sine = math.sin(angle)
    cosine = math.cos(angle)
    x = sine * radius
    z = cosine * radius
    if course == 1:
        return (x * 1.12, z * 0.82)
    if course == 2:
        scale = 1.0 + 0.11 * math.cos(angle * 3.0)
        return (x * scale, z * scale)
    return (x, z)


def make_frame(course, frame, view):
    yaw = frame * math.pi * 2.0 / FRAME_COUNT
    yaw_sine = math.sin(yaw)
    yaw_cosine = math.cos(yaw)
    rings = []
    for radius in (INNER_RADIUS, OUTER_RADIUS):
        ring = []
        for segment in range(SEGMENTS):
            angle = segment * math.pi * 2.0 / SEGMENTS
            x, z = course_point(course, angle, radius)
            rotated_x = x * yaw_cosine + z * yaw_sine
            rotated_z = -x * yaw_sine + z * yaw_cosine
            ring.append(project(view, (rotated_x, 0.0, rotated_z)))
        rings.append(ring)
    return rings


def points_initializer(points):
    return "{ " + ", ".join("{%d, %d}" % point for point in points) + " }"


def generate():
    view = make_view((0.0, 8.5, 15.0), (0.0, 0.0, 0.0),
                     (0.0, 1.0, 0.0))
    courses = [
        [make_frame(course, frame, view) for frame in range(FRAME_COUNT)]
        for course in range(COURSE_COUNT)
    ]
    lines = [
        "#ifndef WDR_COURSDAT_H",
        "#define WDR_COURSDAT_H",
        "",
        "#include \"math3d.h\"",
        "",
        "enum {",
        "  COURSE_PREVIEW_COUNT = %d," % COURSE_COUNT,
        "  COURSE_PREVIEW_FRAME_COUNT = %d," % FRAME_COUNT,
        "  COURSE_PREVIEW_SEGMENTS = %d," % SEGMENTS,
        "};",
        "",
        "struct CoursePreviewFrame {",
        "  Vec2s track[2][COURSE_PREVIEW_SEGMENTS];",
        "};",
        "",
        "static const CoursePreviewFrame",
        "kCoursePreview[COURSE_PREVIEW_COUNT][COURSE_PREVIEW_FRAME_COUNT] = {",
    ]
    for frames in courses:
        lines.append("  {")
        for track in frames:
            lines.extend((
                "    {",
                "      {",
                "        %s," % points_initializer(track[0]),
                "        %s" % points_initializer(track[1]),
                "      },",
                "    },",
            ))
        lines.append("  },")
    lines.extend(("};", "", "#endif", ""))
    return "\n".join(lines)


if __name__ == "__main__":
    print(generate(), end="")
