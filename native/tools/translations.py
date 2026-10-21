"""Maintain GNU gettext catalogs for the native application (no D tooling)."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("extract", "update", "compile"))
    parser.add_argument("--output", type=Path, default=ROOT / "out/i18n")
    parser.add_argument("--gettext-bin", type=Path)
    args = parser.parse_args()

    def tool(name):
        search = str(args.gettext_bin) + os.pathsep + os.environ.get("PATH", "") if args.gettext_bin else None
        result = shutil.which(name, path=search)
        if not result:
            parser.error(f"GNU gettext tool {name} is required; install gettext or pass --gettext-bin")
        return result

    catalogs = sorted((ROOT / "tl").glob("*.po"))
    if not catalogs:
        parser.error("No PO catalogs in tl/")
    if args.command in ("extract", "update"):
        sources = sorted(p.relative_to(ROOT).as_posix() for p in (ROOT / "native/port").rglob("*")
                         if p.suffix in (".cpp", ".hpp"))
        with tempfile.TemporaryDirectory(prefix="nijiexpose-pot-") as directory:
            listing = Path(directory) / "sources.txt"
            listing.write_text("\n".join(sources) + "\n", encoding="utf-8")
            subprocess.run([tool("xgettext"), "--language=C++", "--from-code=UTF-8",
                            "--keyword=_", "--keyword=__", "--keyword=N_", "--keyword=dgettext:2",
                            "--add-comments=TRANSLATORS:", "--sort-by-file",
                            "--package-name=nijiexpose", "--copyright-holder=nijigenerate Project",
                            "--files-from=" + str(listing), "--output=tl/template.pot"], cwd=ROOT, check=True)
        if args.command == "update":
            for catalog in catalogs:
                subprocess.run([tool("msgmerge"), "--update", "--backup=none", str(catalog),
                                str(ROOT / "tl/template.pot")], check=True)
    else:
        args.output.mkdir(parents=True, exist_ok=True)
        for catalog in catalogs:
            subprocess.run([tool("msgfmt"), "--check", "--check-format", str(catalog),
                            "--output-file=" + str(args.output / (catalog.stem + ".mo"))], check=True)


if __name__ == "__main__":
    main()
