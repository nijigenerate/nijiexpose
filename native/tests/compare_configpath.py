import argparse
import os
import pathlib
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--d", required=True)
parser.add_argument("--cpp", required=True)
args = parser.parse_args()
programs = [str(pathlib.Path(path).resolve()) for path in [args.d, args.cpp]]
for mode in ["appdata", "forced", "relative", "fallback", "blocked"]:
    with tempfile.TemporaryDirectory(prefix="nijiexpose-config-") as directory:
        root = pathlib.Path(directory)
        env = dict(os.environ)
        env.pop("IN_CONFIG_PATH", None)
        env["AppData"] = str(root / "日本語 data")
        if mode == "forced": env["IN_CONFIG_PATH"] = str(root / "forced 日本語")
        elif mode == "relative": env["IN_CONFIG_PATH"] = "relative/path"
        elif mode == "fallback": env.pop("AppData", None)
        elif mode == "blocked":
            blocked = root / "blocked"
            blocked.write_bytes(b"fixture")
            env["AppData"] = str(blocked)
        results = [subprocess.run([program], cwd=root, env=env, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, check=True).stdout.splitlines() for program in programs]
        if results[0] != results[1]: raise SystemExit(f"FAIL {mode}: D={results[0]!r} C++={results[1]!r}")
        print(f"PASS D/C++ Windows config paths: {mode}")
