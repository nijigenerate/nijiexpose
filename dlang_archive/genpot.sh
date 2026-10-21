#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
find dlang_archive/source -iname "*.d" | xargs xgettext --from-code=UTF-8 -o tl/template.pot -c --keyword=_ --keyword=__
