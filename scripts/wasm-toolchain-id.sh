#!/usr/bin/env bash
#
# Print the content-derived identifier for the PythonSCAD WASM toolchain.
# With no argument, hash files from the working tree. With a git ref, hash the
# versions stored at that ref. Including file names prevents ambiguous content
# concatenation and keeps CI, local builds, and retention cleanup in agreement.
#
# This is the toolchain-v1 algorithm. Do not change it incompatibly without
# also introducing a new image-tag prefix and retention migration.

set -euo pipefail

REPO_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
readonly REPO_ROOT
cd "$REPO_ROOT"

readonly TOOLCHAIN_FILES=(
  docker/wasm/sysroot.dockerfile
  docker/wasm/requirements.txt
  docker/wasm/emscripten-crossfile.meson
)

git_ref=${1:-}

sha256_stream()
{
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum | awk '{ print $1 }'
  elif command -v shasum >/dev/null 2>&1; then
    shasum -a 256 | awk '{ print $1 }'
  else
    echo "Neither sha256sum nor shasum is available." >&2
    return 1
  fi
}

if [[ -n "$git_ref" ]]; then
  for path in "${TOOLCHAIN_FILES[@]}"; do
    if ! git cat-file -e "${git_ref}:${path}" 2>/dev/null; then
      echo "Toolchain input ${path} is missing at git ref ${git_ref}." >&2
      exit 2
    fi
  done
fi

{
  for path in "${TOOLCHAIN_FILES[@]}"; do
    printf '%s\0' "$path"
    if [[ -n "$git_ref" ]]; then
      content_hash=$(git show "${git_ref}:${path}" | sha256_stream)
    else
      content_hash=$(sha256_stream < "$path")
    fi
    printf '%s' "$content_hash"
    printf '\0'
  done
} | sha256_stream
