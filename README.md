# Wire Drift Racers

[English](README.md) | [日本語](README.ja.md)

<p align="center">
  <strong><a href="https://uraraworks.github.io/WebX68k/?cpu=10&ram=12&fd1=https://raw.githubusercontent.com/renatus-novus-x/wdrfrace/main/dist/wdrfrace.xdf&run=1">Launch Wire Drift Racers in WebX68k</a></strong>
</p>

Wire Drift Racers is a bare-metal 3D wireframe racing game project for the Sharp X68000, written in C++ without Human68k runtime APIs.

## Current Prototype

- Displays a rotating four-edge wireframe car
- Erases the previous frame with black lines before drawing the next frame in white
- Uses `_iocs_line()` for line rendering without a full-screen redraw
- Uses a 256-step trigonometric lookup table
- Measures and displays average FPS over 300 frames
- Currently achieves approximately 15.50 FPS in the tested environment

The current build is a rendering and performance prototype. Racing gameplay and two-player controls are not implemented yet.

## Planned Game

- Offline local two-player racing on one X68000
- Fixed oblique camera over a 3D wireframe ring course
- Acceleration, braking, drifting, lane movement, and limited boost
- Three-lap race or 90-second time-trial rules
- Wireframe-only cars, track walls, ramps, gates, and background grid

## Controls

- `ESC`: exit the prototype

## Build

Requirements: WSL Ubuntu 24.04, an installed `elf2x68k` toolchain, `python3`, and `curl`.

```sh
cd src
make
```

Build outputs:

- `src/human.sys`
- `dist/wdrfrace.xdf`

To inspect the generated XDF:

```sh
cd src
make check-xdf
```

## Documentation

- [Game specification](doc/spec.md)
- [Game specification slides](doc/spec.pptx)
- [Development progress](doc/progress.md)
