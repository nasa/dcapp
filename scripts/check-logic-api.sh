#!/usr/bin/env bash
set -euo pipefail

dcapp_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
generator="$dcapp_root/pilotlight/out/dcapp-genheader"
fixture="$dcapp_root/tests/generated_logic_api.xml"
invalid_identifier_fixture="$dcapp_root/tests/generated_logic_invalid_identifier.xml"
check_source="$dcapp_root/tests/generated_logic_api.c"
c_compiler="${CC:-cc}"
cxx_compiler="${CXX:-c++}"

if [[ ! -x "$generator" ]]; then
    echo "dcapp-genheader is missing; build dcapp before checking the logic API." >&2
    exit 1
fi

check_dir="$(mktemp -d "${TMPDIR:-/tmp}/dcapp-logic-api.XXXXXX")"
trap 'rm -rf "$check_dir"' EXIT

cp "$fixture" "$check_dir/fixture.xml"
"$generator" "$check_dir/fixture.xml"
public_header="$check_dir/logic/dcapp.h"

cp "$invalid_identifier_fixture" "$check_dir/invalid-identifier.xml"
if "$generator" "$check_dir/invalid-identifier.xml" >"$check_dir/invalid-identifier.log" 2>&1; then
    echo "Generator accepted C++ keyword 'class' as an identifier." >&2
    exit 1
fi

require_line() {
    if ! grep -Fq "$1" "$public_header"; then
        echo "Generated logic header is missing: $1" >&2
        exit 1
    fi
}

require_line 'char   (*AbiString)[256];'
require_line 'int    *AbiInteger;'
require_line 'double *AbiDouble;'
require_line 'bool   *AbiBoolean;'
require_line 'double *_AbiLeadingUnderscore;'
require_line 'DCAPP_LOGIC_EXPORT void abi_function(DcAppContext *app_ctx, void *user_data);'
require_line 'DCAPP_LOGIC_EXPORT void abi_draw_function(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args, void *user_data);'

normalize_internal() {
    sed \
        -e 's/DcAppContext/__DC_APP_CONTEXT__/g' \
        -e 's/DcApp/Dc/g' \
        -e 's/__DC_APP_CONTEXT__/DcAppContext/g' \
        -e 's/^[[:space:]]*//' \
        -e 's/[[:space:]][[:space:]]*/ /g'
}

normalize_public() {
    sed \
        -e 's/^[[:space:]]*//' \
        -e 's/[[:space:]][[:space:]]*/ /g'
}

check_table() {
    local internal_type="$1"
    local public_type="$2"
    local internal_header="$3"
    local internal_fields
    local public_fields

    internal_fields="$(
        sed -n "/struct $internal_type {/,/^};/p" "$internal_header" |
            grep -E '\(\*[A-Za-z_][A-Za-z0-9_]*\)' |
            normalize_internal
    )"
    public_fields="$(
        sed -n "/typedef struct $public_type {/,/^} $public_type;/p" "$public_header" |
            grep -E '\(\*[A-Za-z_][A-Za-z0-9_]*\)' |
            normalize_public
    )"

    if [[ "$internal_fields" != "$public_fields" ]]; then
        echo "$public_type does not match $internal_type:" >&2
        diff <(printf '%s\n' "$internal_fields") <(printf '%s\n' "$public_fields") || true
        exit 1
    fi
}

check_table DcAppDrawApi DcDrawApi "$dcapp_root/src/app/draw_api.h"
check_table DcAppMouseApi DcMouseApi "$dcapp_root/src/app/draw_api.h"
check_table DcAppTextureApi DcTextureApi "$dcapp_root/src/app/texture_api.h"
check_table DcAppPlanetApi DcPlanetApi "$dcapp_root/src/app/planet_api.h"
check_table DcAppDisplayLogicApi DcAppApi "$dcapp_root/src/app/display_logic_api.h"

internal_init="$(
    sed -n '/struct DcAppDisplayLogicInit {/,/^};/p' "$dcapp_root/src/app/display_logic_api.h" |
        sed '1d;$d' |
        sed \
            -e 's/DcAppContext/__DC_APP_CONTEXT__/g' \
            -e 's/DcAppDisplayLogicApi/__DC_APP_API__/g' \
            -e 's/DcApp/Dc/g' \
            -e 's/__DC_APP_CONTEXT__/DcAppContext/g' \
            -e 's/__DC_APP_API__/DcAppApi/g' |
        normalize_public
)"
public_init="$(
    sed -n '/typedef struct DcInit {/,/^} DcInit;/p' "$public_header" |
        sed '1d;$d' |
        normalize_public
)"

if [[ "$internal_init" != "$public_init" ]]; then
    echo "DcInit does not match DcAppDisplayLogicInit:" >&2
    diff <(printf '%s\n' "$internal_init") <(printf '%s\n' "$public_init") || true
    exit 1
fi

"$c_compiler" \
    -std=gnu11 \
    -Wall \
    -Wextra \
    -Werror \
    -I"$check_dir/logic" \
    -I"$dcapp_root/src" \
    -fsyntax-only \
    "$check_source"

"$cxx_compiler" \
    -x c++ \
    -std=gnu++17 \
    -Wall \
    -Wextra \
    -Werror \
    -I"$check_dir/logic" \
    -I"$dcapp_root/src" \
    -fsyntax-only \
    "$check_source"

echo "Generated logic API check passed."
