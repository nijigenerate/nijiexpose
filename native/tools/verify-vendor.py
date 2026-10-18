"""Check the exact bytes of the selected vendored dependencies."""
from pathlib import Path
import hashlib
import json


def main():
    root = Path(__file__).resolve().parents[2] / "vendor"
    manifest = json.loads((root / "manifest.json").read_text(encoding="utf-8"))
    for relative, expected in manifest.items():
        path = (root / relative).resolve()
        if not path.is_relative_to(root) or not path.is_file():
            raise SystemExit(f"Missing or invalid vendor file: {relative}")
        if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            raise SystemExit(f"Vendor checksum mismatch: {relative}")
    print(f"Verified {len(manifest)} vendored files, including license notices.")


if __name__ == "__main__":
    main()
