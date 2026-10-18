"""Control a running nijiexpose started with --control-dir DIRECTORY."""
import argparse
import json
import pathlib
import time
import uuid


def request(directory, command, timeout=10):
    directory = pathlib.Path(directory)
    if not directory.is_dir():
        raise RuntimeError("Control directory does not exist; start the app with --control-dir")
    token = uuid.uuid4().hex
    pending = directory / (token + ".request.json")
    response = directory / (token + ".response.json")
    temporary = directory / (token + ".tmp")
    temporary.write_text(json.dumps(command, allow_nan=False), encoding="utf-8")
    temporary.replace(pending)
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if response.exists():
            result = json.loads(response.read_text(encoding="utf-8"))
            response.unlink()
            if not result["ok"]:
                raise RuntimeError(result["error"])
            return result["result"]
        time.sleep(.05)
    # Cancel an unconsumed request; a timeout is not proof the action wasn't applied.
    pending.unlink(missing_ok=True)
    raise TimeoutError("No response. Query list before retrying: the action may have been applied.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", required=True)
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("list")
    parameters = commands.add_parser("parameters")
    parameters.add_argument("index", type=int)
    fit = commands.add_parser("fit")
    fit.add_argument("index", type=int)
    fit.add_argument("--margin", type=float, default=80)
    transform = commands.add_parser("set_transform")
    transform.add_argument("index", type=int)
    for field in ("scale", "x", "y"):
        transform.add_argument("--" + field, type=float)
    args = vars(parser.parse_args())
    directory = args.pop("directory")
    try:
        print(json.dumps(request(directory, {k: v for k, v in args.items() if v is not None}), ensure_ascii=False, indent=2))
    except (RuntimeError, TimeoutError, ValueError) as error:
        parser.exit(1, str(error) + "\n")
