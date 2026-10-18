"""Verify checked-in D comparison artifacts before running parity tests."""
import hashlib
import json
from pathlib import Path
root=Path(__file__).parent
manifest=json.loads((root/"reference-provenance.json").read_text(encoding="utf-8"))
for name,expected in manifest["records"].items():
    actual=hashlib.sha256((root/name).read_bytes()).hexdigest()
    if actual!=expected:
        raise SystemExit(f"Original D comparison artifact changed: {name}")
print(f"Verified {len(manifest['records'])} original D comparison artifact hashes")
