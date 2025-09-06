#!/usr/bin/env bash
#
# Runs a command in the OpenSCAD Base Wasm Docker image for Emscripten builds.
# (mounts $PWD as readwrite and sets up ccache)
#
# Example usage:
#
#   ./scripts/wasm-base-docker-run.sh emcmake cmake -B build-node -DWASM_BUILD_TYPE=node -DCMAKE_BUILD_TYPE=Release -DEXPERIMENTAL=1
#   ./scripts/wasm-base-docker-run.sh cmake --build build-node -j
#   build-node/openscad.js -h
#
# If docker fails because of a platform mismatch, e.g. on Silicon Macs
# (build env currently only built for linux/amd64), register QEMU with Docker using:
#
#   docker run --privileged --rm tonistiigi/binfmt --install all
#
# Also see:
# - https://github.com/openscad/openscad-wasm
#   For a barebones setup
# - https://github.com/openscad/openscad-playground
#   For a full-fledged example
#
set -euo pipefail

CCACHE_DIR=${CCACHE_DIR:-$HOME/.ccache/}
mkdir -p "$CCACHE_DIR"

# TODO: remove this once the base image has ccache installed
echo "
  FROM openscad/wasm-base:latest
  RUN apt update && \
      apt install -y ccache && \
      apt clean
" | docker build \
  --platform=linux/amd64 \
  -t openscad-wasm-ccache:local \
  -f - .

docker run --rm -it \
  --platform=linux/amd64 \
  -w /src \
  -v "$PWD:/src:rw" \
  -v $CCACHE_DIR:/root/.ccache:rw \
  openscad-wasm-ccache:local \
  "$@"
