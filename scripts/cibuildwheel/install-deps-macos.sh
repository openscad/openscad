#!/usr/bin/env bash
# Install native build dependencies for macOS wheel builds.
# Runs as cibuildwheel [tool.cibuildwheel.macos] before-all.
set -euo pipefail

echo "=== install-deps-macos.sh: installing pip wheel build deps ==="

# Homebrew is preinstalled on GitHub-hosted macOS runners.
./scripts/get-dependencies.py --yes --profile pythonscad-pip

echo "=== install-deps-macos.sh: done ==="
