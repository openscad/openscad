#!/bin/bash

# Synchronize submodule versions from another *SCAD repository

if [ $# -ne 1 ]; then
  printf "Synchronize submodule versions from another *SCAD repository\n\n"
  printf "Usage: %s <openscad_path>\n" "$0"
  exit 1
fi

openscad_clone="${1}"

while IFS= read -r line; do
  commit="$(echo \"$line\" | awk '{print $2}')"
  dir="$(echo \"$line\" | awk '{print $3}')"
  (
    cd "$dir" && git checkout "$commit" && git submodule update --init --recursive
  )
done < <(cd "$openscad_clone" && git submodule status)
