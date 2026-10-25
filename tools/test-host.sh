#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cmake -S "$root/tests/host" -B "$root/build-host" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$root/build-host" --parallel
ctest --test-dir "$root/build-host" --output-on-failure
