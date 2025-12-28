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
    # Use the standalone detect_version.sh script (single source of truth)
    "${SCRIPT_DIR}/detect_version.sh"
}

openscad_commit() {
    # Use the standalone detect_commit.sh script (single source of truth)
    "${SCRIPT_DIR}/detect_commit.sh"
}
