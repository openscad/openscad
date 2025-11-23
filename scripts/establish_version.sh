#!/usr/bin/env bash

# Determine the root directory of the repository (one level up from this script)
# Support both bash (BASH_SOURCE) and zsh (using $0 when BASH_SOURCE is unset)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]:-$0}" )" &> /dev/null && pwd )"
ROOT_DIR="${SCRIPT_DIR}/.."

VERSION_FILE="${ROOT_DIR}/VERSION.txt"
COMMIT_FILE="${ROOT_DIR}/COMMIT.txt"

# Functions to retrieve version and commit information
#
# Usage:
#   source scripts/establish_version.sh
#   export OPENSCAD_VERSION=$(openscad_version)
#   export OPENSCAD_COMMIT=$(openscad_commit)

openscad_version() {
    local VERSION=""
    # /VERSION.txt is a file in the root folder containing a single version identifier
    if [ -f "$VERSION_FILE" ]; then
        # Read version identifier, removing any whitespace
        VERSION=$(tr -d '[:space:]' < "$VERSION_FILE")
        if [ -n "$VERSION" ]; then
            echo "Found VERSION.txt file, using VERSION=$VERSION" >&2
            echo "$VERSION"
            return 0
        fi
    fi

    # Default to YYYY.MM.DD of current day
    VERSION=$(date "+%Y.%m.%d")
    echo "No VERSION.txt file found, defaulting VERSION=$VERSION" >&2
    echo "$VERSION"
}

openscad_commit() {
    local COMMIT=""
    # /COMMIT.txt is a git commit
    if [ -f "$COMMIT_FILE" ]; then
        # Read git commit, removing any whitespace
        COMMIT=$(tr -d '[:space:]' < "$COMMIT_FILE")
        if [ -n "$COMMIT" ]; then
            echo "Found COMMIT.txt file, using COMMIT=$COMMIT" >&2
            echo "$COMMIT"
            return 0
        fi
    else
        # Try to get from git
        if command -v git >/dev/null 2>&1; then
            # Check if we are in a git repo
            if git -C "$ROOT_DIR" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
                COMMIT=$(git -C "$ROOT_DIR" log -1 --pretty=format:"%h")
                echo "No COMMIT.txt file found, using git commit: $COMMIT" >&2
                echo "$COMMIT"
                return 0
            fi
        fi
    fi
}
