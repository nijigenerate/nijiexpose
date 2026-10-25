import argparse
import json
import os
import pathlib
import subprocess
import tempfile
parser = argparse.ArgumentParser()
parser.add_argument("--d")
parser.add_argument("--cpp", required=True)
parser.add_argument("--write-reference", action="store_true")
args = parser.parse_args()
reference_file = pathlib.Path(__file__).with_name("settings_reference.json")
reference = [] if args.d else json.loads(reference_file.read_text(encoding="utf-8"))
cases = [None, {}, {"UIScale": 1.5, "lang": "ja", "visible": False},
         {"UIScale": None, "lang": None, "bgColor": None},
         {"UIScale": "2.4", "lang": "", "bgColor": [0.2, 0.3]},
         {"UIScale": True, "lang": 123, "visible": 1, "bgColor": [0.2, "wrong", 0.6]},
         {"UIScale": "nan"}, {"UIScale": "inf"}, {"UIScale": "-inf"},
         {"bgColor": [1, 2, 3, 4, 5]},
         {"window": {"width": 1280, "height": 720}, "pluginStates": {"test": {"running": True}},
          "space": {"zones": [{"name": "名前", "focus": [1, 2, 3]}]}, "unknown": [None, "保持"]},
         "null", "[]", '"leaf"', '{"broken":', '{"a":1,"a":2}']
for index, case in enumerate(cases):
    results = []
    for program in ([args.d, args.cpp] if args.d else [args.cpp]):
        with tempfile.TemporaryDirectory(prefix="nijiexpose-settings-") as directory:
            root = pathlib.Path(directory)
            filename = root / "settings.json"
            if case is not None:
                filename.write_text(case if isinstance(case, str) else json.dumps(case, ensure_ascii=False), encoding="utf-8")
            env = dict(os.environ, IN_CONFIG_PATH=str(root))
            result = subprocess.run([program], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
            saved = None
            if filename.exists():
                try: saved = json.loads(filename.read_text(encoding="utf-8"))
                except json.JSONDecodeError: saved = {"raw": filename.read_bytes().hex()}
            results.append([result.stdout.decode().splitlines(), saved])
    if not args.d: results.insert(0, reference[index])
    if results[0] != results[1]:
        print(index, case, results)
        raise SystemExit("FAIL settings D/Boost.JSON comparison outside authorized JSON formatting differences")
    if args.d: reference.append(results[0])
if args.write_reference:
    assert args.d
    reference_file.write_text(json.dumps(reference, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print(f"PASS: {len(cases)} settings cases: defaults, null, wrong types, arrays, unknown fields, save/reload")
