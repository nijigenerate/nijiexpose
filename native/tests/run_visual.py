"""Keep actual GPU screenshots as test artifacts, including failed checks."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

output=Path(sys.argv[3]).resolve()
output.mkdir(parents=True, exist_ok=True)
fixtures=Path(__file__).parent/'fixtures'
with tempfile.TemporaryDirectory(prefix='nijiexpose-visual-') as directory:
    model=Path(directory)/'fixture.inp'
    shutil.copyfile(fixtures/'original-model.bin',model)
    env=dict(os.environ,IN_CONFIG_PATH=directory)
    subprocess.run([sys.argv[1],str(model),str(fixtures/'plugin-visual'),str(output/'windows-panels-model.png')],env=env,check=True)
with tempfile.TemporaryDirectory(prefix='nijiexpose-dialog-') as directory:
    subprocess.run([sys.argv[2],str(output/'windows-dialog.png')],env=dict(os.environ,IN_CONFIG_PATH=directory),check=True)
