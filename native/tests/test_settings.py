"""Test settings behavior without any legacy implementation or recorded output."""
import argparse
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--cpp", required=True)
args = parser.parse_args()

cases = [None, {}, {"UIScale": 1.5, "lang": "en", "visible": False,
                  "bgColor": [0.25, 0.5, 0.75, 1.0]},
         {"window": {"width": 1280, "height": 720},
          "pluginStates": {"test": {"running": True}},
          "unknown": [None, "保持"]}]
for case in cases:
    with tempfile.TemporaryDirectory(prefix="nijiexpose-settings-") as directory:
        filename = Path(directory) / "settings.json"
        if case is not None:
            filename.write_text(json.dumps(case, ensure_ascii=False), encoding="utf-8")
        output = subprocess.run([args.cpp], env=dict(os.environ, IN_CONFIG_PATH=directory),
                                capture_output=True, check=True).stdout.decode().splitlines()
        assert len(output) == 2 and output[1] == "42 6a61", output
        fields = output[0].split()
        values = case or {}
        assert fields[0] == str(int("UIScale" in values)), fields
        assert fields[1] == str(int(values.get("visible", True))), fields
        expected_scale = struct.unpack("<I", struct.pack("<f", values.get("UIScale", 1.0)))[0]
        assert int(fields[2], 16) == expected_scale, fields
        assert fields[3] == str(int("lang" in values)), fields
        color_start = 4
        if "lang" in values:
            assert fields[4] == values["lang"].encode().hex(), fields
            color_start = 5
        colors = [struct.unpack("<f", struct.pack("<I", int(v, 16)))[0]
                  for v in fields[color_start:]]
        assert colors == values.get("bgColor", [0.5, 0.5, 0.5, 0]), colors
        saved = json.loads(filename.read_text(encoding="utf-8"))
        assert saved == dict(values, new=42, lang="ja"), saved
print("PASS settings defaults, typed values, save/reload and unknown-field preservation")
