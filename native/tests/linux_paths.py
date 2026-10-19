import os
import pathlib
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix="nijiexpose-paths-") as temporary:
    root = pathlib.Path(temporary)
    home, xdg = root / "home", root / "xdg"
    home.mkdir()
    env = dict(os.environ, HOME=str(home))
    env.pop("XDG_CONFIG_HOME", None)
    env.pop("IN_CONFIG_PATH", None)

    def check(expected):
        actual = subprocess.check_output([sys.argv[1]], env=env, text=True)
        assert actual == str(expected), (actual, expected)
        assert expected.is_dir(), expected

    check(home / ".config" / "nijiexpose")
    env["XDG_CONFIG_HOME"] = str(xdg)
    check(xdg / "nijiexpose")
    legacy = xdg / ".nijiexpose"
    legacy.mkdir()
    check(legacy)
    forced = root / "forced" / "nested"
    env["IN_CONFIG_PATH"] = str(forced)
    check(forced)
print("PASS: Linux XDG, HOME fallback, legacy preference and forced configuration paths")
