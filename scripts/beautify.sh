#!/bin/bash

# Reformat C++ code using clang-format

# note: with the -style=file option, clang-format reads the config from ./.clang-format
# See here for the full list of supported options: https://clang.llvm.org/docs/ClangFormatStyleOptions.html
FORMAT_CMD="clang-format -i -style=file"

function reformat_all() {
    find . -name "*.h" -o -name "*.hpp" -o -name "*.cc" -o -name "*.cpp" | xargs $FORMAT_CMD
}

# reformat files that differ from master.
DIFFBASE="origin/master"
function reformat_changed() {
    ANCESTOR=$(git merge-base HEAD "$DIFFBASE")
    FILES=$(git --no-pager diff --name-only $ANCESTOR | grep -E "\.h|\.hpp|\.cc|\.cpp")
    if [ $? -ne 0 ]; then
        echo "No files to format, exiting..."
    else
        echo -e "Reformatting files:\n$FILES"
        echo $FILES | xargs $FORMAT_CMD
    fi
}

# main
if [[ "$#" -eq 1 && "$1" == "--all" ]]; then
    echo "Reformatting all files..."
    reformat_all
else
    echo "Reformatting files that differ from $DIFFBASE..."
    reformat_changed
fi
