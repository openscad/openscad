#!/usr/bin/env bash
#
# Version detection script - single source of truth for version determination
# Can be called from CMake, bash scripts, or other build tools
#
# Output: Prints version string to stdout (e.g., "0.6.0-28-g89c44cac7")
# Exit code: 0 on success, 1 on failure
#
# This script implements the version detection priority:
# 1. git describe (preferred)
# 2. VERSION.txt file (fallback for source distributions)

# Determine script directory (works in both bash and zsh)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]:-$0}" )" &> /dev/null && pwd )"
ROOT_DIR="${SCRIPT_DIR}/.."

VERSION=""

# Try git describe first (preferred method)
if command -v git >/dev/null 2>&1; then
    if git -C "$ROOT_DIR" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        # Get version from git describe (e.g., v0.6.0-28-g89c44cac7)
        VERSION=$(git -C "$ROOT_DIR" describe --tags --always --dirty 2>/dev/null || echo "")
        if [ -n "$VERSION" ]; then
            # Remove leading 'v' if present
            VERSION="${VERSION#v}"
            # Output only the version string
            echo "$VERSION"
            exit 0
        fi
    fi
fi

# Fallback to VERSION.txt file
VERSION_FILE="${ROOT_DIR}/VERSION.txt"
if [ -f "$VERSION_FILE" ]; then
    VERSION=$(tr -d '[:space:]' < "$VERSION_FILE")
    if [ -n "$VERSION" ]; then
        echo "$VERSION"
        exit 0
    fi
fi

# No version found
echo "ERROR: Could not detect version. Please ensure git tags are available or VERSION.txt exists." >&2
exit 1
