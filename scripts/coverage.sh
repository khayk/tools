#!/bin/bash

# Generate HTML coverage reports for one or more test targets.
#
# Usage:
#   ./scripts/coverage.sh                          # all targets
#   ./scripts/coverage.sh core-test               # one target
#   ./scripts/coverage.sh core-test duplicates-test  # selected targets

set -euo pipefail

cd "$(dirname "$0")"/..

ALL_TARGETS=(core-test duplicates-test kidmon-test)
BUILD_DIR="build/coverage"

if [ "$#" -eq 0 ]; then
    targets=("${ALL_TARGETS[@]}")
else
    targets=("$@")
fi

echo "==> Configuring (coverage)"
cmake --preset vcpkg-coverage

echo "==> Building"
cmake --build --preset vcpkg-coverage

for target in "${targets[@]}"; do
    echo "==> Coverage: ${target}"
    cmake --build --preset vcpkg-coverage -t "coverage-${target}"
done

echo ""
echo "Reports:"
for target in "${targets[@]}"; do
    echo "  ${BUILD_DIR}/coverage-${target}/index.html"
done
