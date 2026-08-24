#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."
DCAPP_CLANG_FORMAT="${DCAPP_CLANG_FORMAT:-clang-format-14}"

git ls-files -z -- \
    '*.c' '*.h' '*.cc' '*.cpp' '*.cxx' \
    '*.hh' '*.hpp' '*.hxx' '*.m' '*.mm' \
    ':(exclude)tools/terrain/pl_icons.h' |
    xargs -0 "$DCAPP_CLANG_FORMAT" -i --style=file --
