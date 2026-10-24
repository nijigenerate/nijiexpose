import argparse
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("--d", required=True)
parser.add_argument("--cpp", required=True)
parser.add_argument("--child", required=True)
args = parser.parse_args()
for mode in ["capture", "discard", "missing", "terminate", "rerun"]:
    result = [subprocess.run([parent, mode, args.child], stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, timeout=15, check=True).stdout.splitlines() for parent in [args.d, args.cpp]]
    if result[0] != result[1]:
        raise SystemExit(f"FAIL {mode}: D={result[0]!r}, C++={result[1]!r}")
    print(f"PASS D/C++ Windows subprocess: {mode}")
