#!/usr/bin/env bash

set -u

DCAPP_HOME="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMEOUT_SECONDS="${1:-5}"

if ! [[ "$TIMEOUT_SECONDS" =~ ^[0-9]+([.][0-9]+)?$ ]]; then
    echo "Usage: $0 [timeout_seconds]"
    exit 2
fi

run_with_timeout() {
    local timeout_seconds="$1"
    shift

    if command -v timeout >/dev/null 2>&1; then
        timeout "$timeout_seconds" "$@"
        return $?
    fi

    local marker
    marker="$(mktemp "${TMPDIR:-/tmp}/dcapp-run-samples.XXXXXX")" || return 1
    rm -f "$marker"

    "$@" &
    local cmd_pid=$!

    (
        sleep "$timeout_seconds"
        if kill -0 "$cmd_pid" >/dev/null 2>&1; then
            : > "$marker"
            kill "$cmd_pid" >/dev/null 2>&1
        fi
    ) &
    local timer_pid=$!

    wait "$cmd_pid"
    local status=$?

    kill "$timer_pid" >/dev/null 2>&1
    wait "$timer_pid" >/dev/null 2>&1

    if [ -f "$marker" ]; then
        rm -f "$marker"
        return 124
    fi

    rm -f "$marker"
    return "$status"
}

pass_count=0
fail_count=0
skip_count=0
sample_count=0

for sample_dir in "$DCAPP_HOME"/samples/*; do
    [ -d "$sample_dir" ] || continue
    for sample_xml in "$sample_dir"/*.xml; do
        [ -f "$sample_xml" ] || continue
        sample_count=$((sample_count + 1))
    done
done

if [ "$sample_count" -eq 0 ]; then
    echo "No samples found."
    exit 1
fi

echo "Running $sample_count sample XML files with ${TIMEOUT_SECONDS}s timeout each."
echo ""

for sample_dir in "$DCAPP_HOME"/samples/*; do
    [ -d "$sample_dir" ] || continue
    for sample_xml in "$sample_dir"/*.xml; do
        [ -f "$sample_xml" ] || continue
        sample_name="$(basename "$sample_dir")"
        rel_path="${sample_xml#"$DCAPP_HOME"/}"

        if [ "$sample_name" = "bad-sample" ]; then
            echo "[SKIP] $rel_path (intentional invalid sample)"
            skip_count=$((skip_count + 1))
            continue
        fi

        echo "[RUN ] $rel_path"
        run_with_timeout "$TIMEOUT_SECONDS" "$DCAPP_HOME/bin/dcapp.sh" "$sample_xml"
        status=$?

        if [ "$status" -eq 0 ]; then
            echo "[ OK ] $rel_path exited normally"
            pass_count=$((pass_count + 1))
        elif [ "$status" -eq 124 ]; then
            echo "[ OK ] $rel_path reached timeout"
            pass_count=$((pass_count + 1))
        else
            echo "[FAIL] $rel_path exited with status $status"
            fail_count=$((fail_count + 1))
        fi
        echo ""
    done
done

echo "Summary: $pass_count passed, $fail_count failed, $skip_count skipped."

if [ "$fail_count" -ne 0 ]; then
    exit 1
fi
