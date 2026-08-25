#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

required_version="21.1.8"
case "$(uname -s)" in
    Darwin)
        brew_prefix="$(brew --prefix llvm@21 2>/dev/null || true)"
        DCAPP_CLANG_FORMAT="${DCAPP_CLANG_FORMAT:-$brew_prefix/bin/clang-format}"
        install_command="brew install llvm@21"
        ;;
    Linux)
        DCAPP_CLANG_FORMAT="${DCAPP_CLANG_FORMAT:-clang-format}"
        if [[ -f /etc/redhat-release ]]; then
            install_command="sudo yum module install llvm-toolset"
        else
            install_command="install clang-format $required_version from https://apt.llvm.org/"
        fi
        ;;
    *)
        DCAPP_CLANG_FORMAT="${DCAPP_CLANG_FORMAT:-clang-format}"
        install_command="install clang-format $required_version"
        ;;
esac

if ! "$DCAPP_CLANG_FORMAT" --version 2>/dev/null | grep -q "version $required_version"; then
    echo "clang-format $required_version not found."
    echo "Install with: $install_command"
    exit 1
fi

git ls-files -z -- \
    '*.c' '*.h' '*.cc' '*.cpp' '*.cxx' \
    '*.hh' '*.hpp' '*.hxx' '*.m' '*.mm' \
    ':(exclude)tools/terrain/pl_icons.h' |
    xargs -0 "$DCAPP_CLANG_FORMAT" -i --style=file --
