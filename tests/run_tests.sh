#!/usr/bin/env bash
# Run all tool tests.
# Usage: ./tests/run_tests.sh
# Must be executed from the repository root (where ./aether lives).

set -euo pipefail

TOOL="./aether"
TESTS_DIR="$(dirname "$0")"
PASS=0
FAIL=0
SKIP=0

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

if [[ ! -x "$TOOL" ]]; then
    echo "Error: $TOOL not found. Run 'make' first." >&2
    exit 1
fi

for test_dir in "$TESTS_DIR"/*/; do
    [[ -f "$test_dir/input.c"    ]] || continue
    [[ -f "$test_dir/target.json" ]] || continue
    [[ -f "$test_dir/expected.txt" ]] || continue

    name="$(basename "$test_dir")"
    expected="$(cat "$test_dir/expected.txt" | tr -d '[:space:]')"

    # Run the tool — suppress its stdout/stderr, only keep checking.py output
    if ! "$TOOL" "$test_dir/input.c" "$test_dir/target.json" > /dev/null 2>&1; then
        echo -e "${RED}FAIL${NC}  $name  (tool crashed)"
        FAIL=$((FAIL + 1))
        continue
    fi

    if [[ ! -f checking.py ]]; then
        echo -e "${RED}FAIL${NC}  $name  (checking.py not generated)"
        FAIL=$((FAIL + 1))
        continue
    fi

    # Run Z3, capture first line of output
    z3_output="$(python3 checking.py 2>/dev/null | head -1 | tr -d '[:space:]')"

    # Copy checking.py into the test dir for inspection
    cp checking.py "$test_dir/checking.py"

    if [[ "$z3_output" == "$expected" ]]; then
        echo -e "${GREEN}PASS${NC}  $name  ($z3_output)"
        PASS=$((PASS + 1))
    else
        echo -e "${RED}FAIL${NC}  $name  (expected '$expected', got '$z3_output')"
        FAIL=$((FAIL + 1))
    fi
done

echo ""
echo "Results: ${PASS} passed, ${FAIL} failed, ${SKIP} skipped"

if [[ $FAIL -gt 0 ]]; then
    exit 1
fi
