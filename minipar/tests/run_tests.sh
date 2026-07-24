#!/usr/bin/env bash
# run_tests.sh — Minipar test runner (WSL/Linux)
# Golden files: <name>.stdout (expected stdout; omit for exit-only test)
#               <name>.exit  (expected exit code, default 0)
#               <name>.stdin (optional stdin input)
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BINARY="$PROJECT_DIR/minipar"
TIMEOUT=30
PASS=0
FAIL=0
RED='\033[0;31m'
GREEN='\033[0;32m'
YEL='\033[0;33m'
NC='\033[0m'

run_test() {
    local name="$1"
    local src="$SCRIPT_DIR/${name}.minipar"
    local stdout_golden="$SCRIPT_DIR/${name}.stdout"
    local exit_golden="$SCRIPT_DIR/${name}.exit"
    local stdin_file="$SCRIPT_DIR/${name}.stdin"
    local expected_exit=0
    local stdout_only=false

    if [ ! -f "$src" ]; then
        echo -e "${RED}MISSING${NC} $name (no .minipar)"
        FAIL=$((FAIL + 1))
        return
    fi
    if [ -f "$exit_golden" ]; then
        expected_exit=$(cat "$exit_golden")
    fi
    if [ ! -f "$stdout_golden" ]; then
        stdout_only=true
    fi

    local actual_stdout
    local actual_exit
    local actual_stderr

    if [ -f "$stdin_file" ]; then
        actual_stdout=$(timeout "$TIMEOUT" "$BINARY" "$src" < "$stdin_file" 2>/tmp/minipar_test_stderr); actual_exit=$?
    else
        actual_stdout=$(timeout "$TIMEOUT" "$BINARY" "$src" </dev/null 2>/tmp/minipar_test_stderr); actual_exit=$?
    fi
    actual_stderr=$(cat /tmp/minipar_test_stderr)

    if [ "$actual_exit" -eq 124 ]; then
        echo -e "${RED}TIMEOUT${NC} $name"
        FAIL=$((FAIL + 1))
        return
    fi

    if [ "$actual_exit" -ne "$expected_exit" ]; then
        echo -e "${RED}FAIL${NC} $name (exit: expected=$expected_exit got=$actual_exit)"
        if [ -n "$actual_stderr" ]; then echo "  stderr: $actual_stderr"; fi
        FAIL=$((FAIL + 1))
        return
    fi

    if $stdout_only; then
        echo -e "${GREEN}PASS${NC} $name (exit=$actual_exit)"
        PASS=$((PASS + 1))
        return
    fi

    local expected_stdout
    expected_stdout=$(cat "$stdout_golden")

    if [ "$actual_stdout" != "$expected_stdout" ]; then
        echo -e "${RED}FAIL${NC} $name (stdout mismatch)"
        echo "  Expected:"
        echo "$expected_stdout" | sed 's/^/    > /'
        echo "  Got:"
        echo "$actual_stdout" | sed 's/^/    < /'
        FAIL=$((FAIL + 1))
        return
    fi

    echo -e "${GREEN}PASS${NC} $name"
    PASS=$((PASS + 1))
}

echo "=== Minipar Test Suite ==="
echo "Binary: $BINARY"
echo ""

if [ ! -x "$BINARY" ]; then
    echo "Building..."
    make -C "$PROJECT_DIR" run
fi

shopt -s nullglob
tests=()
for f in "$SCRIPT_DIR"/*.minipar; do
    tests+=("$(basename "$f" .minipar)")
done

if [ ${#tests[@]} -eq 0 ]; then
    echo "No tests found in $SCRIPT_DIR"
    exit 1
fi

for t in "${tests[@]}"; do
    run_test "$t"
done

echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="
[ "$FAIL" -gt 0 ] && exit 1
exit 0
