#!/usr/bin/env python3
"""Compile a small MML subset into an NDSS YM2151 stream-effect bank."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re

RATE = 60
VERSION = 1
CHANNEL = 4
NOTE_CODE = (0x0, 0x1, 0x2, 0x4, 0x5, 0x6,
             0x8, 0x9, 0xA, 0xC, 0xD, 0xE)
SEMITONE = {"c": 0, "d": 2, "e": 4, "f": 5,
            "g": 7, "a": 9, "b": 11}
TOKEN = re.compile(
    r"t\d+|o\d+|l\d+|v\d+|[<>]|[a-gr](?:[+#-])?\d*\.?",
    re.IGNORECASE,
)


@dataclass
class Effect:
    name: str
    tone: tuple[int, int, int, int, int, int]
    mml: str


def be32(value: int) -> bytes:
    return value.to_bytes(4, "big")


def parse_source(path: Path) -> tuple[int, list[Effect]]:
    gain = 0
    effects: list[Effect] = []
    name = ""
    tone: tuple[int, int, int, int, int, int] | None = None
    body: list[str] = []
    for number, raw in enumerate(path.read_text(encoding="ascii").splitlines(), 1):
        line = raw.split(";", 1)[0].strip()
        if not line:
            continue
        if line.lower().startswith("@gain"):
            if name:
                raise ValueError(f"{path}:{number}: @gain inside effect")
            gain = int(line.split()[1], 0)
            continue
        if line.lower().startswith("@effect"):
            if name:
                raise ValueError(f"{path}:{number}: nested @effect")
            fields = line.split()
            if len(fields) != 8:
                raise ValueError(
                    f"{path}:{number}: @effect NAME M1 TL1 D1 M4 TL4 D4"
                )
            name = fields[1].upper()
            tone = tuple(int(value, 0) for value in fields[2:])  # type: ignore
            body = []
            continue
        if line.lower() == "@end":
            if not name or tone is None:
                raise ValueError(f"{path}:{number}: @end without @effect")
            effects.append(Effect(name, tone, "".join(body)))
            name = ""
            tone = None
            body = []
            continue
        if not name:
            raise ValueError(f"{path}:{number}: MML outside effect")
        body.append(line)
    if name:
        raise ValueError(f"{path}: unterminated effect {name}")
    if not effects:
        raise ValueError(f"{path}: no effects")
    if not 0 <= gain <= 127:
        raise ValueError("gain must be 0..127")
    return gain, effects


def add_operator(events: list[tuple[int, int, int]], slot: int,
                 multiple: int, total_level: int, decay: int) -> None:
    offset = slot * 8 + CHANNEL
    events.extend((
        (0, 0x40 + offset, multiple & 0x7f),
        (0, 0x60 + offset, total_level & 0x7f),
        (0, 0x80 + offset, 0x1f),
        (0, 0xa0 + offset, decay & 0x1f),
        (0, 0xc0 + offset, 0x00),
        (0, 0xe0 + offset, 0xff),
    ))


def duration_frames(tempo: int, length: int, dotted: bool) -> int:
    if tempo <= 0 or length <= 0:
        raise ValueError("tempo and note length must be positive")
    numerator = RATE * 60 * 4 * (3 if dotted else 2)
    denominator = tempo * length * 2
    return max(1, (numerator + denominator // 2) // denominator)


def compile_effect(effect: Effect, gain: int) -> bytes:
    m1, tl1, d1, m4, tl4, d4 = effect.tone
    tl1 = max(0, min(127, tl1 - gain))
    tl4 = max(0, min(127, tl4 - gain))
    events: list[tuple[int, int, int]] = [(0, 0x08, CHANNEL)]
    events.append((0, 0x20 + CHANNEL, 0xc7))
    events.append((0, 0x38 + CHANNEL, 0x00))
    add_operator(events, 0, m1, tl1, d1)
    add_operator(events, 1, 2, 0x7f, 0)
    add_operator(events, 2, 1, 0x7f, 0)
    add_operator(events, 3, m4, tl4, d4)

    source = re.sub(r"\s+", "", effect.mml.lower())
    tokens = TOKEN.findall(source)
    if "".join(tokens) != source:
        raise ValueError(f"{effect.name}: unsupported MML near {source}")
    tempo = 300
    octave = 4
    default_length = 16
    volume = 15
    frame = 0
    for token in tokens:
        if token[0] == "t":
            tempo = int(token[1:])
        elif token[0] == "o":
            octave = int(token[1:])
        elif token[0] == "l":
            default_length = int(token[1:])
        elif token[0] == "v":
            volume = max(0, min(15, int(token[1:])))
        elif token == ">":
            octave += 1
        elif token == "<":
            octave -= 1
        else:
            match = re.fullmatch(r"([a-gr])([+#-]?)(\d*)(\.?)", token)
            if match is None:
                raise ValueError(f"{effect.name}: invalid token {token}")
            note, accidental, length_text, dot = match.groups()
            length = int(length_text) if length_text else default_length
            duration = duration_frames(tempo, length, bool(dot))
            events.append((frame, 0x08, CHANNEL))
            if note != "r" and volume != 0:
                semitone = SEMITONE[note]
                if accidental in ("+", "#"):
                    semitone += 1
                elif accidental == "-":
                    semitone -= 1
                note_octave = octave
                while semitone < 0:
                    semitone += 12
                    note_octave -= 1
                while semitone >= 12:
                    semitone -= 12
                    note_octave += 1
                if not 0 <= note_octave <= 7:
                    raise ValueError(f"{effect.name}: octave outside 0..7")
                key_code = (note_octave << 4) | NOTE_CODE[semitone]
                events.extend((
                    (frame, 0x28 + CHANNEL, key_code),
                    (frame, 0x30 + CHANNEL, 0),
                    (frame, 0x08, 0x48 | CHANNEL),
                ))
            frame += duration
    events.append((frame, 0x08, CHANNEL))

    encoded = bytearray()
    previous = 0
    for event_frame, register, value in events:
        delay = event_frame - previous
        if not 0 <= delay <= 0xffff:
            raise ValueError(f"{effect.name}: event delay outside 16-bit range")
        encoded.extend((delay >> 8, delay & 0xff, register, value))
        previous = event_frame
    return be32(frame + 1) + be32(len(events)) + bytes(encoded)


def make_bank(effects: list[Effect], gain: int) -> bytes:
    entries = [compile_effect(effect, gain) for effect in effects]
    header_size = 8 + (len(entries) + 1) * 4
    offsets = []
    cursor = header_size
    for entry in entries:
        offsets.append(cursor)
        cursor += len(entry)
    offsets.append(cursor)
    result = bytearray(b"NDSS")
    result.extend((VERSION, len(entries), 0, 0))
    for offset in offsets:
        result.extend(be32(offset))
    for entry in entries:
        result.extend(entry)
    return bytes(result)


def emit_header(data: bytes, symbol: str, source: Path,
                effects: list[Effect]) -> str:
    guard = re.sub(r"[^A-Za-z0-9]", "_", symbol).upper() + "_H_INCLUDED"
    lines = [
        f"/* Generated from {source.name} by ndpstreamse.py. */",
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        "#include <stdint.h>",
        "",
        f"static const uint8_t {symbol}[] = {{",
    ]
    for offset in range(0, len(data), 12):
        chunk = ", ".join(f"0x{value:02x}" for value in data[offset:offset + 12])
        lines.append("    " + chunk + ",")
    lines.extend((
        "};",
        "",
        f"#define {symbol.upper()}_SIZE ((unsigned int)sizeof({symbol}))",
        f"#define {symbol.upper()}_COUNT {len(effects)}",
        "",
        f"#endif /* {guard} */",
        "",
    ))
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--header", type=Path, required=True)
    parser.add_argument("--symbol", default="ndp_stream_se_data")
    args = parser.parse_args()
    gain, effects = parse_source(args.input)
    data = make_bank(effects, gain)
    args.binary.parent.mkdir(parents=True, exist_ok=True)
    args.header.parent.mkdir(parents=True, exist_ok=True)
    args.binary.write_bytes(data)
    args.header.write_text(
        emit_header(data, args.symbol, args.input, effects),
        encoding="ascii", newline="\n",
    )
    print(f"{args.input}: {len(effects)} effects, {len(data)} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
