#!/usr/bin/env bash
#
# Ensure the `uv` command is available on PATH.
#
# GitHub Actions should use astral-sh/setup-uv instead of this script.
# Release pipelines (AppImage, local maintainer builds) call this helper
# before bundle-runtime-python.sh when uv is not pre-installed.
#
# Usage:
#   ensure-uv.sh [--install]
#
# Without --install: exit 0 if uv is on PATH, else print install hints and exit 1.
# With --install:    install uv via Astral's standalone installer when missing.

set -euo pipefail

die() {
    echo "error: $*" >&2
    exit 1
}

INSTALL=0
if [[ "${1:-}" == "--install" ]]; then
    INSTALL=1
elif [[ -n "${1:-}" ]]; then
    echo "usage: ensure-uv.sh [--install]" >&2
    exit 1
fi

if command -v uv >/dev/null 2>&1; then
    exit 0
fi

if [[ "${INSTALL}" -eq 0 ]]; then
    cat >&2 <<'EOF'
error: uv is not on PATH.

Install uv, then re-run:
  Linux/macOS:  curl -LsSf https://astral.sh/uv/install.sh | sh
  MSYS2 UCRT64: pacboy -S --noconfirm uv:p
  PyPI:         python3 -m pip install --user uv

GitHub Actions should use a SHA-pinned astral-sh/setup-uv action instead of this script.
EOF
    exit 1
fi

case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*)
        cat >&2 <<'EOF'
error: automatic uv install is not supported in this shell.

On MSYS2 UCRT64, run:
  pacboy -S --noconfirm uv:p
EOF
        exit 1
        ;;
esac

[[ -n "${HOME:-}" ]] || die "HOME is unset; export HOME or set UV_INSTALL_DIR explicitly"

UV_INSTALL_DIR="${UV_INSTALL_DIR:-${HOME}/.local/bin}"
export UV_INSTALL_DIR
export PATH="${UV_INSTALL_DIR}:${PATH}"

command -v curl >/dev/null 2>&1 \
    || die "curl is required to install uv automatically"

if ! mkdir -p "${UV_INSTALL_DIR}" 2>/dev/null || [[ ! -w "${UV_INSTALL_DIR}" ]]; then
    die "cannot write uv install directory: ${UV_INSTALL_DIR}"
fi

curl -LsSf https://astral.sh/uv/install.sh | sh

if ! command -v uv >/dev/null 2>&1; then
    echo "error: uv install completed but uv is still not on PATH" >&2
    echo "add ${UV_INSTALL_DIR} to PATH and re-run" >&2
    exit 1
fi
