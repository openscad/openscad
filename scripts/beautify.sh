#!/usr/bin/env bash
set -euo pipefail
# Reformat C++ code using clang-format

# This script can be set directly as a git hook:
# cd .git/hooks/
# ln -s ../../scripts/beautify.sh pre-commit

# Resolve script's real location (follow symlinks)
SCRIPT_DIR="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"

# ROOT_DIR="$(git -C "$SCRIPT_DIR" rev-parse --show-toplevel)"
ROOT_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

DOALL=0
CHECKALL=0
DOSTAGED=0

FORMAT_CMD="clang-format -i --style=file"
CHECK_CMD="$FORMAT_CMD --dry-run --Werror"
VERSION_CMD="clang-format --version"
STAGED_FILES_CMD="git diff --name-only --cached"

# Filter out any files that shouldn't be auto-formatted.
# note: -v flag inverts selection - this tells grep to *filter out* anything
#       that matches the pattern. For testing, you can remove the -v to see
#       which files would have been excluded.
FILTER_CMD="grep -v -E /ext/"
FILTER_EXTENSION_CMD="grep -E [.](h|hpp|cc|cpp)$"

function find_all() {
    find "$ROOT_DIR/src" \( -name "*.h" -o -name "*.hpp" -o -name "*.cc" -o -name "*.cpp" \) -a -not -name findversion.h \
        | $FILTER_CMD
}

function check_all() {
    find_all | xargs $CHECK_CMD
}

function reformat_all() {
    find_all | xargs $FORMAT_CMD
}

# reformat files that differ from master.
DIFFBASE="origin/master"
function reformat_changed() {
    ANCESTOR=$(git merge-base HEAD "$DIFFBASE")
    if FILES=$(git --no-pager diff --name-only "$ANCESTOR" | $FILTER_EXTENSION_CMD | $FILTER_CMD); then
        echo -e "Reformatting files:\n$FILES"
        echo $FILES | xargs $FORMAT_CMD
    else
        echo "No files to format, exiting..."
    fi
}

function reformat_staged() {
    cd "$ROOT_DIR"
    if FILES=$($STAGED_FILES_CMD | $FILTER_EXTENSION_CMD | $FILTER_CMD); then
        echo -e "Reformatting files:\n$FILES"
        echo $FILES | xargs $FORMAT_CMD
    else
        echo "No files to format, exiting..."
    fi
}

# parse options
for PARAM in "$@"
do
  KEY="${PARAM%%=*}"
  if [[ "$PARAM" == *"="* ]]; then
    VALUE="${PARAM#*=}"
  else
    VALUE=""
  fi

  if [ "$KEY" == "--diffbase" ]; then
    [ -z "${VALUE}" ] && echo "script option --diffbase=BASE requires a non-empty value" && exit 1
    DIFFBASE="${VALUE}"
  elif [ "$PARAM" == "--check" ]; then
    CHECKALL=1
  elif [ "$PARAM" == "--all" ]; then
    DOALL=1
  elif [ "$PARAM" == "--staged" ]; then
    DOSTAGED=1
  elif [ "$KEY" == "-h" ]; then
    SCRIPT=$(basename "$0")
cat << EOF
Usage: $SCRIPT [--all|--staged|--check|--diffbase=BASE]

Runs clang-format on files. Default scope is files changed from origin/master.

    --all               format all files
    --staged            format staged files
    --check             check files, do not make changes
    --diffbase=BASE     format files that differ from BASE

EOF
    exit
  fi
done

function execute() {
    # Execute function with done message, version, and preserved return value 
    local FUNCTION MESSAGE RETURN_VALUE
    FUNCTION=$1
    MESSAGE=$2

    echo "$MESSAGE"
    $VERSION_CMD
    
    "$FUNCTION"
    RETURN_VALUE=$?

    echo -n "Completed with "
    $VERSION_CMD
    return $RETURN_VALUE
}

if ((DOALL)); then
    execute reformat_all "Reformatting all files..."
elif ((DOSTAGED)); then
    execute reformat_staged "Reformatting staged files..."
elif ((CHECKALL)); then
    execute check_all "Checking all files..."
else
    execute reformat_changed "Reformatting files that differ from $DIFFBASE..."
fi
