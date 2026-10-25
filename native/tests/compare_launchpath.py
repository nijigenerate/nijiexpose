"""Compare the actual D module/Phobos against the C++ translation."""
import argparse
import pathlib
import random
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--d", required=True)
parser.add_argument("--cpp", required=True)
args = parser.parse_args()
rng = random.Random(0x4E494A49)
whitespace = [chr(c).encode() for c in [*range(9, 14), 32, 0x85, 0xA0, 0x1680,
    *range(0x2000, 0x200B), 0x2028, 0x2029, 0x202F, 0x205F, 0x3000]]
cases = [(op, value) for op in "PS" for value in [b"", b"\0", b'""', b' "x" ',
    b'"x"\0padding', b'a\r\nb\r\nc', b"a\n", b"\n\n", '日本語'.encode()]]
for left in whitespace:
    for right in whitespace:
        for middle in [b"x", b'"x"', '日本語'.encode()]:
            cases.append(("P", left + middle + right))
for _ in range(10000):
    value = bytes(rng.randrange(256) for _ in range(rng.randrange(32)))
    cases.extend((("P", value), ("S", value)))
with tempfile.TemporaryDirectory(prefix="nijiexpose-parity-") as directory:
    root = pathlib.Path(directory)
    file = root / "日本語 space.exe"
    file.write_bytes(b"fixture")
    for path in [str(file), str(root), str(root / "missing.exe"), '"' + str(file) + '"', ""]:
        cases.append(("A", path.encode()))
    data = "".join(op + " " + value.hex() + "\n" for op, value in cases).encode()
    outputs = [subprocess.run([program], input=data, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, check=True).stdout.splitlines() for program in [args.d, args.cpp]]
    if len(outputs[0]) != len(cases) or len(outputs[1]) != len(cases):
        raise SystemExit("Oracle did not return one result per input")
    differences = [(i, cases[i], d, cpp) for i, (d, cpp) in enumerate(zip(*outputs)) if d != cpp]
    if differences:
        for diff in differences[:20]: print(diff)
        raise SystemExit(f"{len(differences)} D/C++ differences")
print(f"PASS: {len(cases)} D/C++ path, UTF-8, embedded-NUL, splitLines and Windows file checks")
