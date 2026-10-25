import argparse
import pathlib
import subprocess
parser = argparse.ArgumentParser()
parser.add_argument("--cpp", required=True)
parser.add_argument("--reference", type=pathlib.Path, required=True)
parser.add_argument("--arg", action="append", default=[])
args = parser.parse_args()
expected = args.reference.read_text(encoding="utf-8-sig").splitlines()
actual = subprocess.run([args.cpp, *args.arg], check=True, stdout=subprocess.PIPE).stdout.decode().splitlines()
if actual != expected:
    for index, (left, right) in enumerate(zip(expected, actual)):
        if left != right: print(index, "D:", left, "C++:", right)
    raise SystemExit("D/C++ output differs")
print(f"PASS: {len(actual)} original D output records")
