import os
import subprocess
import tempfile
import sys
with tempfile.TemporaryDirectory(prefix="nijiexpose-ui-state-") as directory:
    subprocess.run(sys.argv[1:], env=dict(os.environ, IN_CONFIG_PATH=directory), check=True)
