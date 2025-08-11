#!/usr/bin/env bash
set -euo pipefail
# Reformat C++ code using clang-format

# This script can be set directly as a git hook:
# cd .git/hooks/
# ln -s ../../scripts/hard_beautify.sh pre-commit

# Resolve script's real location (follow symlinks)
SCRIPT_DIR="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
ROOT_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

ARTIFACTS_DIR="$ROOT_DIR/build/artifacts"
ZIP_FILE="$ARTIFACTS_DIR/1/beautify-patch/beautify-patch.zip"

[ -d "$ARTIFACTS_DIR" ] && rm -r "$ARTIFACTS_DIR"

act -j Beautify -P ubuntu-latest=ghcr.io/catthehacker/ubuntu:act-24.04 --artifact-server-path "$ROOT_DIR/build/artifacts/" || true

if [ -f "$ZIP_FILE" ]; then
  ( 
    cd "$ARTIFACTS_DIR" 
    unzip "$ZIP_FILE"
  )
  ( 
    cd "$ROOT_DIR"
    git apply "$ARTIFACTS_DIR/beautify.patch"
  )
fi
