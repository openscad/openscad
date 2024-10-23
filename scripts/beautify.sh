#!/usr/bin/env bash

# Reformat C++ code using uncrustify

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR=$SCRIPT_DIR/..

FORMAT_CMD_UNCRUSTIFY="uncrustify -c "$ROOT_DIR/.uncrustify.cfg" --no-backup"
FORMAT_CMD=$FORMAT_CMD_UNCRUSTIFY

# Filter out any files that shouldn't be auto-formatted.
# note: -v flag inverts selection - this tells grep to *filter out* anything
#       that matches the pattern. For testing, you can remove the -v to see
#       which files would have been excluded.
FILTER_CMD="grep -v -E ext/"

function reformat_all() {
    find "$ROOT_DIR/src" \( -name "*.h" -o -name "*.hpp" -o -name "*.cc" -o -name "*.cpp" \) -a -not -name findversion.h \
        | $FILTER_CMD \
        | xargs $FORMAT_CMD
}

# reformat files that differ from master.
DIFFBASE="origin/master"
function reformat_changed() {
    ANCESTOR=$(git merge-base HEAD "$DIFFBASE")
    FILES=$(git --no-pager diff --name-only $ANCESTOR | grep -E "\.(h|hpp|cc|cpp)" | $FILTER_CMD)
    if [ $? -ne 0 ]; then
        echo "No files to format, exiting..."
    else
        echo -e "Reformatting files:\n$FILES"
        echo $FILES | xargs $FORMAT_CMD
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
    [ -z "$var" ] && echo "script option --diffbase=BASE requires a non-empty value" && exit 1
    DIFFBASE="${VALUE}"
  elif [ "$PARAM" == "--all" ]; then
    DOALL=1
  elif [ "$KEY" == "-h" ]; then
    SCRIPT=$(basename "$0")
    echo "Runs uncrustify on files which differ from diffbase, OR across the entire project (for --all)"
    echo "If no options given, then diffbase defaults to \"origin/master\""
    echo
    echo "Usage:"
    echo "    $SCRIPT [--all|--diffbase=BASE]"
    echo
    exit
  fi
done


if ((DOALL)); then
    echo "Reformatting all files..."
    reformat_all
else
    echo "Reformatting files that differ from $DIFFBASE..."
    reformat_changed
fi
