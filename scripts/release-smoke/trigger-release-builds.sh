#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
# shellcheck source=common-smoke.sh
source "$SCRIPT_DIR/common-smoke.sh"

RELEASE_BRANCH="release-please--branches--master--components--pythonscad"

usage() {
  cat <<'EOF'
Usage: trigger-release-builds.sh [options]

Dispatch all packaging workflows needed for release smoke testing.

Requires Bash 4 or newer for --wait progress tracking.

Options:
  --repo <owner/name>          GitHub repository to use. Defaults to current repo.
  --ref <branch-or-tag>        Ref to dispatch. Defaults to the release-please
                               branch when it exists, otherwise master.
  --upload-to-release <tag>    Pass upload_to_release to packaging workflows.
  --wait                       Wait until all dispatched workflows finish.
  --dry-run                    Print workflow dispatch commands without running them.
  --help                       Show this help.

Triggered workflows:
  build-appimage.yml
  build-debian-packages.yml
  build-rpm-packages.yml
  build-windows-native.yml
  build-macos-release.yml
EOF
}

repo=""
ref=""
upload_to_release=""
wait_for_runs=0
dry_run=0

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
    --upload-to-release)
      upload_to_release=${2:-}
      [[ -n "$upload_to_release" ]] || rs_die "--upload-to-release requires a value"
      shift 2
      ;;
    --wait)
      wait_for_runs=1
      shift
      ;;
    --dry-run)
      dry_run=1
      shift
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

rs_require_command gh

if [[ -z "$repo" ]]; then
  repo=$(rs_repo_default)
fi

if [[ -z "$ref" ]]; then
  if gh api "repos/${repo}/branches/${RELEASE_BRANCH}" >/dev/null 2>&1; then
    ref=$RELEASE_BRANCH
  else
    ref=master
  fi
fi

case "$ref" in
  *\"*|*\\*|*$'\n'*|*$'\r'*)
    rs_die "--ref contains characters that cannot be used in GitHub Actions jq filters"
    ;;
esac

workflows=(
  build-appimage.yml
  build-debian-packages.yml
  build-rpm-packages.yml
  build-windows-native.yml
  build-macos-release.yml
)

find_dispatched_run_id() {
  local workflow=$1
  local created_after=$2

  gh run list \
    --repo "$repo" \
    --workflow "$workflow" \
    --event workflow_dispatch \
    --limit 20 \
    --json 'databaseId,createdAt,headBranch' \
    --jq "map(select(.createdAt >= \"$created_after\" and .headBranch == \"$ref\")) | sort_by(.createdAt) | .[0].databaseId // \"\""
}

wait_for_dispatched_run_id() {
  local workflow=$1
  local created_after=$2
  local run_id=""

  for _ in {1..30}; do
    run_id=$(find_dispatched_run_id "$workflow" "$created_after")
    if [[ -n "$run_id" ]]; then
      printf '%s\n' "$run_id"
      return 0
    fi
    sleep 10
  done

  return 1
}

wait_for_workflows() {
  local -A completed=()
  local remaining=${#run_ids[@]}
  local failures=0

  rs_log "Waiting for $remaining workflow runs; polling once per minute"

  while ((remaining > 0)); do
    for workflow in "${workflows[@]}"; do
      if [[ -n "${completed[$workflow]:-}" ]]; then
        continue
      fi

      local run_id=${run_ids[$workflow]}
      local run_info
      run_info=$(gh run view "$run_id" \
        --repo "$repo" \
        --json status,conclusion,url,workflowName \
        --jq '[.status, (.conclusion // ""), .url, .workflowName] | @tsv')

      local status conclusion url workflow_name
      IFS=$'\t' read -r status conclusion url workflow_name <<< "$run_info"

      if [[ "$status" == "completed" ]]; then
        completed[$workflow]=1
        ((remaining -= 1))
        rs_log "Finished $workflow_name ($workflow): ${conclusion:-unknown} - $url"
        if [[ "$conclusion" != "success" ]]; then
          failures=1
        fi
      fi
    done

    if ((remaining > 0)); then
      rs_log "$remaining workflow run(s) still running"
      sleep 60
    fi
  done

  if ((failures)); then
    rs_die "one or more dispatched workflows failed"
  fi

  rs_log "All dispatched workflows completed successfully"
}

rs_log "Repository: $repo"
rs_log "Ref: $ref"
if [[ -n "$upload_to_release" ]]; then
  rs_log "upload_to_release: $upload_to_release"
fi

if [[ "$wait_for_runs" == "1" ]]; then
  declare -A run_ids=()
fi

for workflow in "${workflows[@]}"; do
  cmd=(gh workflow run "$workflow" --repo "$repo" --ref "$ref")
  if [[ -n "$upload_to_release" ]]; then
    cmd+=(--field "upload_to_release=$upload_to_release")
  fi

  if [[ "$dry_run" == "1" ]]; then
    printf 'DRY-RUN:'
    printf ' %q' "${cmd[@]}"
    printf '\n'
  else
    rs_log "Dispatching $workflow"
    dispatch_started=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    "${cmd[@]}"
    if [[ "$wait_for_runs" == "1" ]]; then
      if run_id=$(wait_for_dispatched_run_id "$workflow" "$dispatch_started"); then
        run_ids[$workflow]=$run_id
        rs_log "Tracking $workflow as run $run_id"
      else
        rs_die "could not find dispatched run for $workflow"
      fi
    fi
  fi
done

if [[ "$wait_for_runs" == "1" && "$dry_run" == "0" ]]; then
  wait_for_workflows
fi
