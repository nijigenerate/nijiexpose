"""Check every literal assignment in the unchanged D neSetStyle against C++ runtime values."""
import argparse
import pathlib
import re
import struct
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("--source", required=True)
parser.add_argument("--cpp", required=True)
args = parser.parse_args()
text = pathlib.Path(args.source).read_text(encoding="utf-8")
expected = {}
for key, vector, scalar in re.findall(r"style\.(Colors\[ImGuiCol\.\w+\]|\w+)\s*=\s*(?:ImVec[24]\(([^;]+)\)|([\d.]+f))\s*;", text):
    values = vector.split(",") if vector else [scalar]
    for i, value in enumerate(values):
        name = key + ("." + "xyzw"[i] if vector else "")
        expected[name] = struct.pack("<f", float(value.strip().removesuffix("f"))).hex()
for i, value in enumerate(re.search(r"uiSetColor\(UIColor.ButtonTextColor, ImVec4\(([^)]+)\)", text).group(1).split(",")):
    expected["ButtonTextColor." + "xyzw"[i]] = struct.pack("<f", float(value.strip())).hex()
output = subprocess.run([args.cpp], stdout=subprocess.PIPE, check=True, text=True).stdout
actual = {key: bytes.fromhex(value)[::-1].hex() for key, value in (line.split("=") for line in output.splitlines())}
if actual != expected:
    raise SystemExit(f"D/C++ style assignment difference: {[(k, v, actual.get(k)) for k, v in expected.items() if actual.get(k) != v]}")
print(f"PASS: all {len(expected)} D neSetStyle float assignments match C++ bit-for-bit")
