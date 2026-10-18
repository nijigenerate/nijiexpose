import argparse
import json
import os
import pathlib
import struct
import subprocess
import tempfile
parser = argparse.ArgumentParser()
parser.add_argument("--cpp", required=True)
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix="nijiexpose-gettext-") as directory:
    root = pathlib.Path(directory)
    catalog = root / "i18n" / "ja.mo"
    catalog.parent.mkdir()
    entries = [("", "Content-Type: text/plain; charset=UTF-8\nLanguage: ja\n"),
               ("LANG_NAME", "日本語"), ("hello", "こんにちは")]
    sources, targets, strings = [], [], bytearray()
    offset = 28 + len(entries) * 16
    for source, _ in entries:
        value = source.encode()
        sources.append((len(value), offset + len(strings)))
        strings.extend(value + b"\0")
    for _, target in entries:
        value = target.encode()
        targets.append((len(value), offset + len(strings)))
        strings.extend(value + b"\0")
    japanese = (struct.pack("<7I", 0x950412de, 0, len(entries), 28, 28 + len(entries)*8, 0, 0)
                + b"".join(struct.pack("<2I", *entry) for entry in sources + targets) + strings)
    catalog.write_bytes(japanese)
    # GNUTranslations is used only as an independent reader to validate the fixture.
    import gettext
    with catalog.open("rb") as stream:
        assert gettext.GNUTranslations(stream).gettext("hello") == "こんにちは"
    de_entries = [("", "Content-Type: text/plain; charset=UTF-8\nLanguage: de\n"),
                  ("LANG_NAME", "Deutsch"), ("hello", "Hallo")]
    de_sources, de_targets, de_strings = [], [], bytearray()
    for source, _ in de_entries:
        data = source.encode()
        de_sources.append((len(data), offset + len(de_strings)))
        de_strings.extend(data + b"\0")
    for _, target in de_entries:
        data = target.encode()
        de_targets.append((len(data), offset + len(de_strings)))
        de_strings.extend(data + b"\0")
    (catalog.parent / "de.mo").write_bytes(struct.pack("<7I", 0x950412de, 0, 3, 28, 52, 0, 0)
        + b"".join(struct.pack("<2I", *entry) for entry in de_sources + de_targets) + de_strings)
    (root / "settings.json").write_text(json.dumps({"lang": "ja", "unknown": [1, True]}), encoding="utf-8")
    subprocess.run([args.cpp, str(catalog)], env=dict(os.environ, IN_CONFIG_PATH=str(root)), check=True)
    saved = json.loads((root / "settings.json").read_text(encoding="utf-8"))
    assert saved == {"lang": "", "unknown": [1, True]}, saved
    for extra_env in [dict(LC_ALL="C", LC_CTYPE="C", LANG="C"), dict(LANG="de_DE.UTF-8", LANGUAGE="de")]:
        (root / "settings.json").write_text(json.dumps({"lang": "ja", "unknown": [1, True]}), encoding="utf-8")
        subprocess.run([args.cpp, str(catalog)], env=dict(os.environ, IN_CONFIG_PATH=str(root), **extra_env), check=True)
