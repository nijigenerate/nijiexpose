import argparse
import os
import pathlib
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--d", required=True)
parser.add_argument("--cpp", required=True)
args = parser.parse_args()
for mode in ["new", "append", "rotate", "invalid", "unset"]:
    results = []
    for program in [args.d, args.cpp]:
        with tempfile.TemporaryDirectory(prefix="nijiexpose-log-") as directory:
            root = pathlib.Path(directory)
            log = root / "nijiexpose" / "logs" / "nijiexpose.log"
            if mode in ["append", "rotate"]:
                log.parent.mkdir(parents=True)
                log.write_bytes(b"old\n" if mode == "append" else b"x" * (5 * 1024 * 1024))
                log.with_suffix(".log.previous").write_bytes(b"older")
            env = dict(os.environ)
            if mode == "unset": env.pop("LOCALAPPDATA", None)
            elif mode == "invalid":
                blocked = root / "blocked"
                blocked.write_bytes(b"not a directory")
                env["LOCALAPPDATA"] = str(blocked)
            else: env["LOCALAPPDATA"] = str(root)
            completed = subprocess.run([program], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
            results.append((completed.stdout, completed.stderr, log.read_bytes() if log.exists() else None,
                log.with_suffix(".log.previous").read_bytes() if log.with_suffix(".log.previous").exists() else None))
    if results[0] != results[1]:
        for index, (d, cpp) in enumerate(zip(*results)):
            if d != cpp: print(index, repr(d[:1000] if d else d), repr(cpp[:1000] if cpp else cpp))
        raise SystemExit(f"FAIL D/C++ logging: {mode}")
    print(f"PASS D/C++ Windows logging: {mode}")
