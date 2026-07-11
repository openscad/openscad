#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
# shellcheck source=common-smoke.sh
source "$SCRIPT_DIR/common-smoke.sh"

usage() {
  cat <<'EOF'
Usage: test-appimages.sh [options]

Download and smoke test the latest x86_64 AppImage artifacts.

Options:
  --repo <owner/name>       GitHub repository to query. Defaults to current repo.
  --ref <branch-or-tag>     Branch used for latest-run filtering.
  --run-id <id>             Use a specific build-appimage.yml run.
  --workdir <path>          Working directory. Defaults to a temp directory.
  --keep-workdir            Keep temporary files after the run.
  --skip-download           Use artifacts already present in the artifact dir.
  --artifact-dir <path>     Directory containing downloaded AppImages.
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

workdir=$(rs_make_workdir "$workdir_arg")
trap 'rs_cleanup_workdir "$workdir" "$keep_workdir" "$?"' EXIT

if [[ -z "$artifact_dir" ]]; then
  artifact_dir="$workdir/artifacts"
fi

if [[ "$skip_download" == "0" ]]; then
  rs_require_command gh
  if [[ -z "$repo" ]]; then
    repo=$(rs_repo_default)
  fi

  if [[ -z "$run_id" ]]; then
    run_id=$(rs_latest_successful_run "$repo" build-appimage.yml "$ref")
    [[ -n "$run_id" && "$run_id" != "null" ]] || rs_die "no successful build-appimage.yml run found"
  fi
  rs_log "Downloading AppImage artifacts from run $run_id"
  rs_download_run_artifact "$repo" "$run_id" 'pythonscad-qt*-x86_64-appimage' "$artifact_dir"
else
  rs_log "Using existing AppImage artifacts in $artifact_dir"
fi

appimages=()
while IFS= read -r -d '' appimage; do
  appimages+=("$appimage")
done < <(find "$artifact_dir" -type f -name '*.AppImage' ! -name '*aarch64*' ! -name '*arm64*' -print0)

((${#appimages[@]} > 0)) || rs_die "no x86_64 AppImages found in $artifact_dir"

export APPIMAGE_EXTRACT_AND_RUN="${APPIMAGE_EXTRACT_AND_RUN:-1}"

for appimage in "${appimages[@]}"; do
  chmod +x "$appimage"
  rs_smoke_binary "$appimage" "$(basename "$appimage")" "$workdir"
done

rs_log "All AppImage smoke tests passed"
