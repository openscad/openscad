#!/usr/bin/env bash

set -euo pipefail

rs_log() {
  printf '==> %s\n' "$*"
}

rs_warn() {
  printf 'WARNING: %s\n' "$*" >&2
}

rs_die() {
  printf 'ERROR: %s\n' "$*" >&2
  exit 1
}

rs_require_command() {
  local cmd=$1
  command -v "$cmd" >/dev/null 2>&1 || rs_die "required command not found: $cmd"
}

rs_file_nonempty() {
  local path=$1
  local phase=${2:-output verification}
  if [[ ! -e "$path" ]]; then
    rs_die "smoke test failed: $phase; expected output file does not exist: $path"
  fi
  if [[ ! -s "$path" ]]; then
    rs_die "smoke test failed: $phase; expected non-empty output file: $path"
  fi
}

rs_print_log_excerpt() {
  local log_file=$1
  if [[ ! -e "$log_file" ]]; then
    printf -- '--- command output log was not created: %s ---\n' "$log_file" >&2
  elif [[ -s "$log_file" ]]; then
    printf -- '--- begin command output: %s ---\n' "$log_file" >&2
    cat "$log_file" >&2
    printf -- '--- end command output: %s ---\n' "$log_file" >&2
  else
    printf -- '--- command output was empty: %s ---\n' "$log_file" >&2
  fi
}

rs_print_command() {
  printf '%q' "$1"
  shift
  while (($#)); do
    printf ' %q' "$1"
    shift
  done
  printf '\n'
}

rs_repo_default() {
  gh repo view --json nameWithOwner --jq .nameWithOwner
}

rs_latest_successful_run() {
  local repo=$1
  local workflow=$2
  local ref=${3:-}
  local args=(run list --repo "$repo" --workflow "$workflow" --status success
    --json 'databaseId,headBranch')
  if [[ -n "$ref" ]]; then
    case "$ref" in
      *\"*|*\\*|*$'\n'*|*$'\r'*)
        rs_die "--ref contains characters that cannot be used in GitHub Actions jq filters"
        ;;
    esac
    args+=(--limit 50 --jq "map(select(.headBranch == \"$ref\")) | .[0].databaseId // \"\"")
  else
    args+=(--limit 1 --jq '.[0].databaseId')
  fi
  gh "${args[@]}"
}

rs_download_run_artifact() {
  local repo=$1
  local run_id=$2
  local pattern=$3
  local dest=$4

  mkdir -p "$dest"
  gh run download "$run_id" --repo "$repo" --pattern "$pattern" --dir "$dest"
}

rs_make_workdir() {
  local explicit_workdir=$1
  if [[ -n "$explicit_workdir" ]]; then
    mkdir -p "$explicit_workdir"
    printf '%s\n' "$explicit_workdir"
  else
    mktemp -d "${TMPDIR:-/tmp}/pythonscad-release-smoke.XXXXXX"
  fi
}

rs_cleanup_workdir() {
  local workdir=$1
  local keep_workdir=$2
  local status=${3:-0}
  if [[ "$keep_workdir" == "1" ]]; then
    rs_log "Keeping workdir: $workdir"
  elif [[ "$status" != "0" ]]; then
    rs_warn "A failure occurred; keeping workdir for logs: $workdir"
  else
    rm -rf "$workdir"
  fi
}

rs_write_common_test_inputs() {
  local testdir=$1

  mkdir -p "$testdir"
  printf 'cube(10);\n' > "$testdir/cube.scad"
  cat > "$testdir/cube.py" <<'PY'
from pythonscad import *

cube(10).show()
PY
  cat > "$testdir/repl-smoke.py" <<'PY'
from pythonscad import *

export(cube(10), "repl-cube.stl")
raise SystemExit(0)
PY
  cat > "$testdir/ipython-smoke.py" <<'PY'
from pythonscad import *

export(cube(10), "ipython-cube.stl")
PY
}

rs_run_logged() {
  local phase=$1
  local log_file=$2
  shift 2

  set +e
  QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}" "$@" > "$log_file" 2>&1
  local status=$?
  set -e

  if [[ "$status" != "0" ]]; then
    {
      printf 'ERROR: smoke test failed: %s\n' "$phase"
      printf 'Exit code: %s\n' "$status"
      printf 'Command: '
      rs_print_command "$@"
    } >&2
    rs_print_log_excerpt "$log_file"
    return "$status"
  fi
}

rs_run_logged_in_dir() {
  local phase=$1
  local workdir=$2
  local log_file=$3
  local stdin_file=$4
  shift 4

  set +e
  if [[ -n "$stdin_file" ]]; then
    (
      cd "$workdir" &&
        QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}" "$@" < "$stdin_file"
    ) > "$log_file" 2>&1
  else
    (
      cd "$workdir" &&
        QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}" "$@"
    ) > "$log_file" 2>&1
  fi
  local status=$?
  set -e

  if [[ "$status" != "0" ]]; then
    {
      printf 'ERROR: smoke test failed: %s\n' "$phase"
      printf 'Working directory: %s\n' "$workdir"
      if [[ -n "$stdin_file" ]]; then
        printf 'Stdin: %s\n' "$stdin_file"
      fi
      printf 'Exit code: %s\n' "$status"
      printf 'Command: '
      rs_print_command "$@"
    } >&2
    rs_print_log_excerpt "$log_file"
    return "$status"
  fi
}

rs_smoke_binary() {
  local exe=$1
  local label=$2
  local workdir=$3

  [[ -x "$exe" ]] || rs_die "executable is not runnable: $exe"

  local testdir="$workdir/smoke-${label//[^A-Za-z0-9_.-]/_}"
  rs_write_common_test_inputs "$testdir"

  rs_log "Smoke testing $label: startup"
  rs_run_logged "$label: startup" "$testdir/info.log" "$exe" --info

  rs_log "Smoke testing $label: OpenSCAD .scad export"
  rs_run_logged "$label: OpenSCAD .scad export" "$testdir/scad-export.log" \
    "$exe" -o "$testdir/cube-scad.stl" "$testdir/cube.scad"
  rs_file_nonempty "$testdir/cube-scad.stl" "$label: OpenSCAD .scad export"

  rs_log "Smoke testing $label: Python CLI export"
  rs_run_logged "$label: Python CLI export" "$testdir/python-export.log" \
    "$exe" --trust-python -o "$testdir/cube-python.stl" "$testdir/cube.py"
  rs_file_nonempty "$testdir/cube-python.stl" "$label: Python CLI export"

  rs_log "Smoke testing $label: basic Python REPL"
  rs_run_logged_in_dir "$label: basic Python REPL" "$testdir" "$testdir/repl.log" \
    "$testdir/repl-smoke.py" "$exe" --repl
  rs_file_nonempty "$testdir/repl-cube.stl" "$label: basic Python REPL"

  rs_log "Smoke testing $label: IPython"
  rs_run_logged_in_dir "$label: IPython" "$testdir" "$testdir/ipython.log" "" \
    "$exe" --ipython ipython-smoke.py
  rs_file_nonempty "$testdir/ipython-cube.stl" "$label: IPython"

  rs_log "Smoke testing $label: OK"
}
