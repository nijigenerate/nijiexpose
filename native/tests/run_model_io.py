import pathlib
import shutil
import subprocess
import sys
import tempfile
with tempfile.TemporaryDirectory(prefix="nijiexpose-model-io-") as work:
    model=pathlib.Path(work)/"reference.inp"
    shutil.copyfile(pathlib.Path(__file__).parent/"fixtures/original-model.bin",model)
    subprocess.run([sys.argv[1],str(model)],check=True)
