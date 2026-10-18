import subprocess
import sys
import tempfile
with tempfile.TemporaryDirectory(prefix="nijiexpose-plugin-") as work:
    subprocess.run([sys.argv[1], work], check=True)
