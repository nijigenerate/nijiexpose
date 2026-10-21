# Native gettext catalogs

The application reads GNU gettext `.mo` catalogs. `ja.po` is the editable
Japanese catalog; `template.pot` is extracted from C++ code in `native/port`.
No D source or D build tool is used for extraction or compilation.

```sh
# Regenerate the POT and merge current strings into all PO files.
python3 native/tools/translations.py update
# Compile checked catalogs for distribution or a host without msgfmt.
python3 native/tools/translations.py compile --output out/i18n
```

Use `--gettext-bin <directory>` if GNU gettext's tools are not on PATH.
`genpot.sh` extracts the POT only, and `gentl.sh` compiles catalogs. The
equivalent CMake maintenance targets are `translations-extract` and
`translations-update`. Review fuzzy/new translations after merging; gettext
does not use fuzzy entries until a translator approves them.

Normal application builds compile changed PO files with `msgfmt --check
--check-format` and refresh the executable's catalogs even when no C++ source
changed. Installation and CPack include these generated files in `bin/i18n`
on Windows/Linux, or `Contents/Resources/i18n` on macOS. Each catalog must
translate `LANG_NAME`, which is used by the language selector.
