#!/usr/bin/env bash
#
# Sorts the CMake-style file list(s) given as arguments by most recently
# modified first.
#
# Note: not using `ls -t` as it drops the folder from the output. Here we
# preserve the file paths.
#
set -eu

function get_timestamp() {
  # Try Unix & BSD (Mac) syntaxes
  stat -c%Y "$1" 2>/dev/null || stat -f%c "$1"
}

function get_timestamps() {
	for f in "$@" ; do
		echo "$(get_timestamp "$f") $f"
	done
}

# Source paths are relative to the project root.
cd $( dirname -- "${BASH_SOURCE[0]}" )/..

# # - Split the input path(s) by semicolon
# # - Prefix each of them with their epoch timestamp
# # - Sort (numerically) by that timestamp, then drop it
# # - Join the paths
get_timestamps $( echo "$@" | tr ';' ' ' ) | \
  sort -rh | \
  sed -E 's/[^ ]+ //g' | \
  tr '\n' ' ' | \
  sed 's/ /;/g'
