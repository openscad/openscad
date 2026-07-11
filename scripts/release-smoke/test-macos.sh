#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
# shellcheck source=common-smoke.sh
source "$SCRIPT_DIR/common-smoke.sh"

usage() {
  cat <<'EOF'
Usage: test-macos.sh [options]

Download, mount, and smoke test the latest macOS DMG artifact.

Options:
  --repo <owner/name>       GitHub repository to query. Defaults to current repo.
  --ref <branch-or-tag>     Branch used for latest-run filtering.
  --run-id <id>             Use a specific build-macos-release.yml run.
  --workdir <path>          Working directory. Defaults to a temp directory.
  --keep-workdir            Keep temporary files after the run.
  --skip-download           Use artifacts already present in the artifact dir.
  --artifact-dir <path>     Directory containing a downloaded DMG.
  --help                    Show this help.
EOF
}

repo=""
ref=""
run_id=""
workdir_arg=""
keep_workdir=0
skip_download=0
artifact_dir=""
mountpoint=""

while (($#)); do
  case "$1" in
    --repo)
      repo=${2:-}
      [[ -n "$repo" ]] || rs_die "--repo requires a value"
      shift 2
      ;;
    --ref)
      ref=${2:-}
      [[ -n "$ref" ]] || rs_die "--ref requires a value"
      shift 2
      ;;
    --run-id)
      run_id=${2:-}
      [[ -n "$run_id" ]] || rs_die "--run-id requires a value"
      shift 2
      ;;
    --workdir)
      workdir_arg=${2:-}
      [[ -n "$workdir_arg" ]] || rs_die "--workdir requires a value"
      shift 2
      ;;
    --keep-workdir)
      keep_workdir=1
      shift
      ;;
    --skip-download)
      skip_download=1
      shift
      ;;
    --artifact-dir)
      artifact_dir=${2:-}
      [[ -n "$artifact_dir" ]] || rs_die "--artifact-dir requires a value"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      rs_die "unknown option: $1"
      ;;
  esac
done

rs_require_command find
rs_require_command hdiutil

workdir=$(rs_make_workdir "$workdir_arg")
cleanup() {
  local status=$?
  if [[ -n "$mountpoint" && -d "$mountpoint" ]]; then
    hdiutil detach "$mountpoint" >/dev/null 2>&1 || true
  fi
  rs_cleanup_workdir "$workdir" "$keep_workdir" "$status"
  return "$status"
}
trap cleanup EXIT

if [[ -z "$artifact_dir" ]]; then
  artifact_dir="$workdir/artifacts"
fi

if [[ "$skip_download" == "0" ]]; then
  rs_require_command gh
  if [[ -z "$repo" ]]; then
    repo=$(rs_repo_default)
  fi

  if [[ -z "$run_id" ]]; then
    run_id=$(rs_latest_successful_run "$repo" build-macos-release.yml "$ref")
    [[ -n "$run_id" && "$run_id" != "null" ]] || rs_die "no successful build-macos-release.yml run found"
  fi
  rs_log "Downloading macOS DMG artifact from run $run_id"
  rs_download_run_artifact "$repo" "$run_id" 'PythonSCAD-*' "$artifact_dir"
else
  rs_log "Using existing macOS artifacts in $artifact_dir"
fi

dmgs=()
while IFS= read -r -d '' dmg; do
  dmgs+=("$dmg")
done < <(find "$artifact_dir" -type f -name '*.dmg' -print0)

((${#dmgs[@]} > 0)) || rs_die "no DMG found in $artifact_dir"
((${#dmgs[@]} == 1)) || rs_die "expected exactly one DMG in $artifact_dir; found ${#dmgs[@]}"

mountpoint="$workdir/dmg-mount"
mkdir -p "$mountpoint"
rs_log "Mounting $(basename "${dmgs[0]}")"
hdiutil attach "${dmgs[0]}" -readonly -nobrowse -mountpoint "$mountpoint" >/dev/null

shopt -s nullglob
app_candidates=("$mountpoint"/*.app)
shopt -u nullglob

apps=()
for app in "${app_candidates[@]}"; do
  [[ -d "$app" ]] && apps+=("$app")
done

((${#apps[@]} > 0)) || rs_die "no .app bundle found in mounted DMG"
((${#apps[@]} == 1)) || rs_die "expected exactly one .app bundle in DMG; found ${#apps[@]}"

app=${apps[0]}
app_name=$(basename "$app" .app)
exe=""
if [[ -x "$app/Contents/MacOS/$app_name" ]]; then
  exe="$app/Contents/MacOS/$app_name"
elif [[ -x "$app/Contents/MacOS/pythonscad" ]]; then
  exe="$app/Contents/MacOS/pythonscad"
else
  for candidate in "$app"/Contents/MacOS/*; do
    if [[ -f "$candidate" && -x "$candidate" && "$candidate" != *-python ]]; then
      exe=$candidate
      break
    fi
  done
fi

[[ -n "$exe" ]] || rs_die "could not locate PythonSCAD executable inside $app"
rs_smoke_binary "$exe" "$(basename "$app")" "$workdir"

rs_log "macOS DMG smoke tests passed"
