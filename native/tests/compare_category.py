"""Compare category geometry without requiring CPU-identical RSQRTSS results.

The raw fixture is authenticated against every original D frame hash before use.
Only position components allow 0.001 pixel error; UVs, colors, indices, counts
and category state remain exact. ImGui uses approximate SSE reciprocal square
root to normalize anti-aliasing fringe normals, whose bits vary across CPUs.
"""
import argparse
import math
from pathlib import Path
import struct
import subprocess
import tempfile


def records(data):
    offset = 0
    while offset < len(data):
        frame, vertices, indices = struct.unpack_from("<III", data, offset)
        offset += 12
        end = offset + vertices * 20 + indices * 2
        if end > len(data):
            raise ValueError("Truncated geometry")
        yield frame, vertices, indices, data[offset:end]
        offset = end


def fnv(data):
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & 0xffffffffffffffff
    return f"{value:016x}"


def compare(expected, actual):
    if len(expected) != len(actual):
        raise ValueError("Draw-list count differs")
    maximum = 0.0
    changed = 0
    for left, right in zip(expected, actual):
        if left[:3] != right[:3]:
            raise ValueError("Frame or vertex/index count differs")
        frame, count, _, baseline = left
        rendered = right[3]
        for index in range(count):
            start = index * 20
            if baseline[start + 8:start + 20] != rendered[start + 8:start + 20]:
                raise ValueError(f"Frame {frame} vertex {index}: UV/color differs")
            for a, b in zip(struct.unpack_from("<ff", baseline, start),
                            struct.unpack_from("<ff", rendered, start)):
                error = abs(a - b)
                if not math.isfinite(error) or error > 0.001:
                    raise ValueError(f"Frame {frame} vertex {index}: position {a} != {b}")
                maximum = max(maximum, error)
                changed += a != b
        if baseline[count * 20:] != rendered[count * 20:]:
            raise ValueError(f"Frame {frame}: indices differ")
    return maximum, changed


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cpp", required=True)
    args = parser.parse_args()
    root = Path(__file__).parent
    lines = (root / "category_reference.txt").read_text().splitlines()
    expected = list(records((root / "category_geometry.bin").read_bytes()))
    if len(expected) != len(lines):
        raise ValueError("Reference frame count differs")
    for (frame, count, _, data), line in zip(expected, lines):
        state, vertices, checksum = line.split()
        if frame >= len(lines) or count != int(vertices) or fnv(data) != checksum:
            raise ValueError("Geometry fixture does not match original D hash")
    with tempfile.TemporaryDirectory() as temp:
        output = Path(temp) / "geometry.bin"
        result = subprocess.run([args.cpp, str(output)], check=True, capture_output=True, text=True)
        actual_lines = result.stdout.splitlines()
        if [line.split()[:2] for line in actual_lines] != [line.split()[:2] for line in lines]:
            raise ValueError("Category states or vertex counts differ")
        actual = list(records(output.read_bytes()))
    maximum, changed = compare(expected, actual)
    print(f"PASS: D-authenticated geometry; {changed} position components differ; max error {maximum:.9g} px; UV/color/index exact")


if __name__ == "__main__":
    main()
