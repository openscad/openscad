#!/bin/bash

# Reformat C++ code using clang-format

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR=$SCRIPT_DIR/..

# note: with the -style=file option, clang-format reads the config from ./.clang-format
# See here for the full list of supported options: https://clang.llvm.org/docs/ClangFormatStyleOptions.html
FORMAT_CMD_CLANG_FORMAT="clang-format -i -style=file"
FORMAT_CMD_UNCRUSTIFY="uncrustify -c "$ROOT_DIR/.uncrustify.cfg" --no-backup"
FORMAT_CMD=$FORMAT_CMD_UNCRUSTIFY

# Filter out any files that shouldn't be auto-formatted.
# note: -v flag inverts selection - this tells grep to *filter out* anything
#       that matches the pattern. For testing, you can remove the -v to ssee
#       which files would have been excluded.
FILTER_CMD="grep -v -E libtess2/|polyclipping/|CGAL_Nef3_workaround.h|CGAL_workaround_Mark_bounded_volumes.h|convex_hull_3_bugfix.h|OGL_helper.h|lodepng.h|lodepng.cpp"

function reformat_all() {
    find "$ROOT_DIR/src" -name "*.h" -o -name "*.hpp" -o -name "*.cc" -o -name "*.cpp" \
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

# main
if [ "`echo $* | grep clang`" ]; then
    FORMAT_CMD=$FORMAT_CMD_CLANG_FORMAT
fi
if [ "`echo $* | grep -- --all`" ]; then
    echo "Reformatting all files..."
    reformat_all
else
    echo "Reformatting files that differ from $DIFFBASE..."
    reformat_changed
fi
