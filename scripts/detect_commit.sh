#!/usr/bin/env bash
#
# Commit detection script - single source of truth for commit hash determination
# Can be called from CMake, bash scripts, or other build tools
#
# Output: Prints short commit hash to stdout (e.g., "89c44cac7")
# Exit code: 0 on success, 1 if no commit found
#
# This script implements the commit detection priority:
# 1. COMMIT.txt file (for source distributions)
# 2. git log (for git repositories)

# Determine script directory (works in both bash and zsh)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]:-$0}" )" &> /dev/null && pwd )"
ROOT_DIR="${SCRIPT_DIR}/.."

COMMIT=""

# Try COMMIT.txt file first
COMMIT_FILE="${ROOT_DIR}/COMMIT.txt"
if [ -f "$COMMIT_FILE" ]; then
    COMMIT=$(tr -d '[:space:]' < "$COMMIT_FILE")
    if [ -n "$COMMIT" ]; then
        echo "$COMMIT"
        exit 0
    fi
fi

# Try to get from git
if command -v git >/dev/null 2>&1; then
    if git -C "$ROOT_DIR" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        COMMIT=$(git -C "$ROOT_DIR" log -1 --pretty=format:"%h" 2>/dev/null || echo "")
        if [ -n "$COMMIT" ]; then
            echo "$COMMIT"
            exit 0
        fi
    fi
fi

# No commit found
exit 1
