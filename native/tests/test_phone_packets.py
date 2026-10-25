"""A malformed datagram must not terminate either phone receiver thread."""
import socket
import subprocess
import sys
import time

for mode in ("ifm", "fm3d"):
    process = subprocess.Popen([sys.argv[1], mode, "49983"], stdin=subprocess.PIPE,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        assert process.stdout.readline().strip() == "READY"
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sender:
            for packet in (b" ", b"=|", b"=head#1,2|", b"leftEye#1,2,3,4|"):
                sender.sendto(packet, ("127.0.0.1", 49983))
                time.sleep(.03)
            sender.sendto(b"=head#10,20,30,1,2,3|leftEye#2,3,4|rightEye#5,6,7|",
                          ("127.0.0.1", 49983))
            time.sleep(.1)
        output, errors = process.communicate("1\nquit\n", timeout=10)
        assert process.returncode == 0, (output, errors)
        assert "state 1 1" in output and "bone Head" in output and "STOPPED" in output, output
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()
print("Both phone receivers survive malformed UDP and receive the next valid frame")
