import pathlib
import socket
import struct
import subprocess
import sys
import tempfile

receiver, writer = sys.argv[1:]
with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
    sock.bind(("127.0.0.1", 0))
    port = sock.getsockname()[1]
process = subprocess.Popen([receiver, str(port)], stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE, text=True)
try:
    assert process.stdout.readline().strip() == "ready"
    valid = b"/alive\0\0,i\0\0" + struct.pack(">i", 123)
    nested = valid
    for _ in range(200):
        nested = b"#bundle\0" + bytes(8) + struct.pack(">I", len(nested)) + nested
    bad = [b"", b"bad", b"/bad\0\0\0\0,z\0\0", b"#bundle\0",
           b"#bundle\0" + bytes(8) + b"\xff\xff\xff\xff", nested]
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        for packet in bad:
            sock.sendto(packet, ("127.0.0.1", port))
        sock.sendto(valid, ("127.0.0.1", port))
    out, err = process.communicate(timeout=15)
    assert process.returncode == 0 and "alive" in out, (process.returncode, out, err)
finally:
    if process.poll() is None:
        process.kill()
        process.wait()
print("PASS: malformed datagrams and deeply nested bundle; receiver remains usable")

def osc_string(value):
    data = value.encode() + b"\0"
    return data + bytes((-len(data)) % 4)

def message(path, tags, arguments=b""):
    return osc_string(path) + osc_string("," + tags) + arguments

for mode in ("vmc", "phiz"):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.bind(("127.0.0.1", 0))
        port = sock.getsockname()[1]
    process = subprocess.Popen([receiver, str(port), mode], stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, text=True)
    try:
        assert process.stdout.readline().strip() == "ready"
        if mode == "vmc":
            packets = [
                message("/VMC/Ext/Key", ""),
                message("/VMC/Ext/Key", "sss", osc_string("x") * 3),
                message("/VMC/Ext/Blend/Val", "f", bytes(4)),
                message("/VMC/Ext/Bone/Pos/Head", "fffffff", bytes(28)),
                message("/VMC/Ext/Blend/Val", "sf", osc_string("alive") + struct.pack(">f", .5)),
            ]
        else:
            packets = [
                message("/phiz/headRotation", ""),
                message("/phiz/headRotation", "ssss", osc_string("x") * 4),
                message("/phiz/blendshapes", "f", bytes(4)),
                message("/phiz/headRotation", "ffff", struct.pack(">ffff", 0, 0, 0, 1)),
            ]
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            for packet in packets:
                sock.sendto(packet, ("127.0.0.1", port))
        out, err = process.communicate(timeout=15)
        assert process.returncode == 0 and "alive" in out, (mode, process.returncode, out, err)
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()
print("PASS: invalid VMC/Phiz arguments rejected; subsequent valid tracking received")

with tempfile.TemporaryDirectory(prefix="nijiexpose-security-") as root:
    root = pathlib.Path(root)
    inbox = root / "inbox"
    inbox.mkdir()
    victim = root / "outside.txt"
    victim.write_text("sentinel")
    result = inbox / "audit.response.json"
    temporary = inbox / "audit.response.json.tmp"

    def run(expected):
        p = subprocess.run([writer, str(inbox)], timeout=10)
        assert p.returncode == expected, p.returncode
        assert victim.read_text() == "sentinel"

    run(0)
    assert result.read_text() == '{"ok":true}'
    # Existing results must not be truncated either.
    run(1)
    result.unlink()
    for target in (temporary, result):
        target.hardlink_to(victim)
        run(1)
        target.unlink()
        try:
            target.symlink_to(victim)
        except OSError as exc:
            if sys.platform != "win32":
                raise
            print("SKIP: Windows symlink creation unavailable:", exc)
        else:
            run(1)
            target.unlink()
    run(0)
print("PASS: exclusive response creation; existing files and link targets unchanged")
