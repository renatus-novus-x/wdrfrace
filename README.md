# Wire Drift Racers

[English](README.md) | [日本語](README.ja.md)

<p align="center">
  <strong><a href="https://uraraworks.github.io/WebX68k/?cpu=10&ram=12&fd1=https://raw.githubusercontent.com/renatus-novus-x/wdrfrace/main/dist/wdrfrace.xdf&run=1">Launch Wire Drift Racers in WebX68k</a></strong>
</p>

Wire Drift Racers is a bare-metal 3D wireframe battle-racing game for the
Sharp X68000. It is written in C++, boots directly from XDF, and does not use
the Human68k runtime.

## Features

- Local 1-player race against a five-level CPU or offline 2-player battle
- Three selectable wireframe courses: RING, OVAL, and PULSE
- Three-lap races with boost, active gates, drift tackles, catch-up recovery,
  and slipstream
- Fixed 20 Hz game simulation and double-buffered 60 Hz display timing
- Title, course select, controls, demo replay, race, result, and sound-test modes
- Low-CPU NDP STREAM BGM and precomputed STREAM sound effects
- Keyboard and two-gamepad controls

## Controls

| Action | Player 1 | Player 2 | Gamepad |
|---|---|---|---|
| Accelerate / decelerate | `W` / `S` | Cursor up/down | Up/down |
| Drift | `A` / `D` | Cursor left/right | Left/right |
| Boost | `Q` | `N` | Button 1 |
| Brake | `E` | `M` | Button 2 |

On menus, use up/down to select, left/right to change a value, and `SPACE` or
Button 1 to confirm. `ESC` or Button 2 cancels where available. Press `D` on
the test screen to toggle the FPS and coordinate-axis display.

## Build

Requirements: WSL Ubuntu 24.04, [elf2x68k](https://github.com/yunkya2/elf2x68k),
GNU Make, Python 3, and `curl`.

```sh
cd src
make clean
make
```

The default build is the public build. It never includes locally owned NDP
DemoSongs, even if `src/bgmpriv.h` exists. It generates:

- `src/human.sys`
- `dist/wdrfrace.xdf`

To build a private local XDF after supplying your own licensed STREAM song
headers and `src/bgmpriv.h`, opt in explicitly and use a different output name:

```sh
cd src
make clean
make PRIVATE_BGM=1 XDF=../dist/wdrpriv.xdf
```

Never publish `bgmpriv.h`, generated private song headers, `wdrpriv.xdf`, or
an XDF containing purchased DemoSongs.

## Sound integration

[`src/ndp.h`](src/ndp.h) is the NDP X68000 single-header player. BGM uses the
low-CPU `NDP_PROFILE_STREAM` profile and is updated from the 60 Hz V-DISP
interrupt. Original game sound effects are defined in
[`src/wdr_se.mml`](src/wdr_se.mml), compiled into an `NDSS` bank during the
build, and played on channels reserved from the BGM stream.

The public build includes the original SE but no purchased BGM. See
[ndp-x68k](https://github.com/renatus-novus-x/ndp-x68k) for the standalone
STREAM BGM + STREAM SE sample and data-generation tools.

## Documentation

- [Instruction manual](doc/manual-en.md)
- [Japanese instruction manual](doc/manual-ja.md)
- [Game specification](doc/spec.md)
- [Development roadmap](doc/roadmap.md)
- [Development progress](doc/progress.md)

## License

Source code is released under the MIT License. Original game SE MML is
CC0-1.0. NDP DemoSongs and other purchased music are not part of this
repository or its public build. See [LICENSE_NOTES.md](LICENSE_NOTES.md) and
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
