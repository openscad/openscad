#!/usr/bin/env bash
set -euo pipefail

# Download and extract a MSYS2 mingw-w64 python package for MXE cross-compilation.
#
# Usage:
#   download-msys2-python.sh <repo_url> <package_filename> <output_dir>
#
# Example:
#   download-msys2-python.sh \
#     https://mirror.msys2.org/mingw/ucrt64 \
#     mingw-w64-ucrt-x86_64-python-3.13.11-4-any.pkg.tar.zst \
#     /path/to/source/python_mingw

repo_url="${1:?Usage: $0 <repo_url> <package_filename> <output_dir>}"
package_filename="${2:?Missing package filename}"
output_dir="${3:?Missing output directory}"

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

rm -rf "$output_dir"
mkdir -p "$output_dir"

cd "$output_dir"

echo "Downloading ${repo_url}/${package_filename}"
curl -fsSLO "${repo_url}/${package_filename}"

# Extract to output_dir; archive contains e.g. ucrt64/... paths
bsdtar -xf "$package_filename"

# Keep the output directory clean
rm -f "$package_filename"

"${script_dir}/libpython_patch.sh"

echo "MSYS2 python extracted and patched in: $output_dir"
