#!/usr/bin/env bash

# ==============================================================================
# ANIMATED GIF GENERATOR WITH SMOOTH MORPH TRANSITIONS
# ==============================================================================
# Builds the homepage box animation GIF used on the docs site.
#
# USAGE (from repo root):
#   ./scripts/generate-GUI-animated-gif-for-website.sh [frames-dir] [output-gif]
#
#   frames-dir  Directory containing frame*.png keyframes (default: current dir)
#   output-gif  Destination GIF path (default: web/docs/pictures/box_anim.gif)
#
# Prepare keyframes named frame01.png, frame02.png, ... in frames-dir.
# Requires ImageMagick (convert).
# ==============================================================================

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)

FRAMES_DIR=${1:-.}
OUTPUT_GIF=${2:-"$REPO_ROOT/web/docs/pictures/box_anim.gif"}

STATIC_MS=1500
TRANSITION_TOTAL_MS=100
NUM_TRANSITION_FRAMES=3

DELAY_STATIC=$((STATIC_MS / 10))
DELAY_TRANS=$(((TRANSITION_TOTAL_MS / NUM_TRANSITION_FRAMES) / 10))

if ! command -v convert >/dev/null 2>&1; then
  echo "$0: ImageMagick 'convert' not found in PATH" >&2
  exit 1
fi

if [ ! -d "$FRAMES_DIR" ]; then
  echo "$0: frames directory not found: $FRAMES_DIR" >&2
  exit 1
fi

cd "$FRAMES_DIR"

shopt -s nullglob
frames=(frame*.png)
shopt -u nullglob

if [ "${#frames[@]}" -lt 2 ]; then
  echo "$0: need at least 2 frame*.png files in $(pwd)" >&2
  exit 1
fi

mapfile -t sorted_frames < <(printf '%s\n' "${frames[@]}" | sort -V)
first_file="${sorted_frames[0]}"

cleanup() {
  rm -f morphed_*.png
}
trap cleanup EXIT

convert -delay "$DELAY_TRANS" "${sorted_frames[@]}" "$first_file" \
  -morph "$NUM_TRANSITION_FRAMES" morphed_%03d.png

gif_args=(convert)
count=0
step=$((NUM_TRANSITION_FRAMES + 1))
last_static_idx=-1

for file in morphed_*.png; do
  if [ $((count % step)) -eq 0 ]; then
    gif_args+=(-delay "$DELAY_STATIC" "$file")
    last_static_idx=$((${#gif_args[@]} - 1))
  else
    gif_args+=(-delay "$DELAY_TRANS" "$file")
  fi
  count=$((count + 1))
done

if [ "$last_static_idx" -ge 0 ]; then
  unset "gif_args[$last_static_idx]"
  unset "gif_args[$((last_static_idx - 1))]"
  unset "gif_args[$((last_static_idx - 2))]"
fi

mkdir -p "$(dirname "$OUTPUT_GIF")"
"${gif_args[@]}" -loop 0 "$OUTPUT_GIF"

echo "Done! Wrote $(realpath "$OUTPUT_GIF")"
