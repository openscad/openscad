#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_ROOT=$(cd -- "$SCRIPT_DIR/../.." && pwd)
# shellcheck source=common-smoke.sh
source "$SCRIPT_DIR/common-smoke.sh"

usage() {
  cat <<'EOF'
Usage: test-linux-packages.sh [options]

Download and smoke test x86_64/amd64 .deb and .rpm package artifacts in Docker.

Options:
  --repo <owner/name>       GitHub repository to query. Defaults to current repo.
  --ref <branch-or-tag>     Branch used for latest-run filtering.
  --run-id <id>             Use the same workflow run id for both package types.
  --deb-run-id <id>         Use a specific build-debian-packages.yml run.
  --rpm-run-id <id>         Use a specific build-rpm-packages.yml run.
  --workdir <path>          Working directory. Defaults to a temp directory.
  --keep-workdir            Keep temporary files after the run.
  --skip-download           Use artifacts already present in the artifact dir.
  --artifact-dir <path>     Directory containing downloaded .deb/.rpm files.
  --help                    Show this help.
EOF
}

repo=""
ref=""
deb_run_id=""
rpm_run_id=""
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
      deb_run_id=${2:-}
      rpm_run_id=${2:-}
      [[ -n "$deb_run_id" ]] || rs_die "--run-id requires a value"
      shift 2
      ;;
    --deb-run-id)
      deb_run_id=${2:-}
      [[ -n "$deb_run_id" ]] || rs_die "--deb-run-id requires a value"
      shift 2
      ;;
    --rpm-run-id)
      rpm_run_id=${2:-}
      [[ -n "$rpm_run_id" ]] || rs_die "--rpm-run-id requires a value"
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

rs_require_command docker
rs_require_command find
rs_require_command python3
rs_require_command realpath

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

  if [[ -z "$deb_run_id" ]]; then
    deb_run_id=$(rs_latest_successful_run "$repo" build-debian-packages.yml "$ref")
    [[ -n "$deb_run_id" && "$deb_run_id" != "null" ]] || rs_die "no successful build-debian-packages.yml run found"
  fi
  if [[ -z "$rpm_run_id" ]]; then
    rpm_run_id=$(rs_latest_successful_run "$repo" build-rpm-packages.yml "$ref")
    [[ -n "$rpm_run_id" && "$rpm_run_id" != "null" ]] || rs_die "no successful build-rpm-packages.yml run found"
  fi

  rs_log "Downloading Debian package artifacts from run $deb_run_id"
  rs_download_run_artifact "$repo" "$deb_run_id" 'pythonscad-*-deb' "$artifact_dir/deb"
  rs_log "Downloading RPM package artifacts from run $rpm_run_id"
  rs_download_run_artifact "$repo" "$rpm_run_id" 'pythonscad-*-rpm' "$artifact_dir/rpm"
else
  rs_log "Using existing Linux package artifacts in $artifact_dir"
fi

packages=()
while IFS= read -r -d '' package; do
  packages+=("$package")
done < <(find "$artifact_dir" -type f \( -name '*.deb' -o -name '*.rpm' \) \
  ! -name '*arm64*' ! -name '*aarch64*' -print0)

((${#packages[@]} > 0)) || rs_die "no x86_64/amd64 Linux packages found in $artifact_dir"

package_container_image() {
  local package=$1
  python3 - "$PROJECT_ROOT/supported-distributions.json" "$(basename "$package")" <<'PY'
import json
import re
import sys

config_path, package_name = sys.argv[1], sys.argv[2]
with open(config_path, encoding="utf-8") as handle:
    config = json.load(handle)

target_family = None
target_codename = None
target_version = None

deb_match = re.match(r"pythonscad_.+_([^_]+)_amd64\.deb$", package_name)
if deb_match:
    target_codename = deb_match.group(1)
else:
    rpm_match = re.search(r"\.(fc|el)([0-9]+)\.(x86_64|amd64)\.rpm$", package_name)
    if rpm_match:
        target_family = "fedora" if rpm_match.group(1) == "fc" else "el"
        target_version = rpm_match.group(2)

for dist in config["distributions"]:
    if "amd64" not in dist.get("architectures", []):
        continue
    if target_codename and dist["codename"] == target_codename:
        print(dist["docker_image"])
        sys.exit(0)
    if target_family and dist["family"] == target_family and dist["version"] == target_version:
        print(dist["docker_image"])
        sys.exit(0)

raise SystemExit(f"no docker image mapping for {package_name}")
PY
}

smoke_image_tag() {
  local package_type=$1
  local base_image=$2
  python3 - "$package_type" "$base_image" <<'PY'
import hashlib
import re
import sys

package_type, base_image = sys.argv[1], sys.argv[2]
digest = hashlib.sha256(base_image.encode("utf-8")).hexdigest()[:16]
package_type = re.sub(r"[^a-z0-9_.-]", "-", package_type.lower())
print(f"pythonscad-release-smoke-{package_type}:{digest}")
PY
}

ensure_deb_smoke_image() {
  local base_image=$1
  local smoke_image
  smoke_image=$(smoke_image_tag deb "$base_image")

  if docker image inspect "$smoke_image" >/dev/null 2>&1; then
    rs_log "Using cached Debian smoke image $smoke_image" >&2
  else
    rs_log "Building Debian smoke image $smoke_image" >&2
    docker build --build-arg BASE_IMAGE="$base_image" -t "$smoke_image" - <<'DOCKERFILE'
ARG BASE_IMAGE=debian:stable-slim
FROM ${BASE_IMAGE}
ENV DEBIAN_FRONTEND=noninteractive
RUN set -eux; \
  apt-get update; \
  apt-get install -y --no-install-recommends ca-certificates libgl1-mesa-dri python3-ipython xauth xvfb
DOCKERFILE
  fi

  printf '%s\n' "$smoke_image"
}

ensure_rpm_smoke_image() {
  local base_image=$1
  local smoke_image
  smoke_image=$(smoke_image_tag rpm "$base_image")

  if docker image inspect "$smoke_image" >/dev/null 2>&1; then
    rs_log "Using cached RPM smoke image $smoke_image" >&2
  else
    rs_log "Building RPM smoke image $smoke_image" >&2
    docker build --build-arg BASE_IMAGE="$base_image" -t "$smoke_image" - <<'DOCKERFILE'
ARG BASE_IMAGE=fedora:latest
FROM ${BASE_IMAGE}
RUN set -eux; \
  if command -v dnf >/dev/null 2>&1; then \
    if [ -f /etc/redhat-release ] && ! [ -f /etc/fedora-release ]; then \
      dnf install -y epel-release dnf-plugins-core || true; \
      dnf config-manager --set-enabled crb || true; \
    fi; \
    dnf install -y mesa-dri-drivers python3-ipython xauth xorg-x11-server-Xvfb; \
    dnf clean all; \
  else \
    yum install -y mesa-dri-drivers python3-ipython xauth xorg-x11-server-Xvfb; \
    yum clean all; \
  fi
DOCKERFILE
  fi

  printf '%s\n' "$smoke_image"
}

run_deb_package_test() {
  local package=$1
  local base_image=$2
  local smoke_dir=$3
  local image
  local package_file
  image=$(ensure_deb_smoke_image "$base_image")
  package_file=$(basename "$package")

  docker run --rm \
    -v "$package:/tmp/pythonscad-package.deb:ro" \
    -v "$smoke_dir:/tmp/pythonscad-smoke" \
    -v "$SCRIPT_DIR:/release-smoke:ro" \
    -e DEBIAN_FRONTEND=noninteractive \
    -e PACKAGE_FILE="$package_file" \
    "$image" \
    bash -c '
      set -euo pipefail
      apt-get update
      cd /tmp
      apt-get install -y ./pythonscad-package.deb
      QT_QPA_PLATFORM=xcb xvfb-run -a bash -c '"'"'
        source /release-smoke/common-smoke.sh
        rs_smoke_binary /usr/bin/pythonscad "$PACKAGE_FILE" /tmp/pythonscad-smoke
      '"'"'
    '
}

run_rpm_package_test() {
  local package=$1
  local base_image=$2
  local smoke_dir=$3
  local image
  local package_file
  image=$(ensure_rpm_smoke_image "$base_image")
  package_file=$(basename "$package")

  docker run --rm \
    -v "$package:/tmp/pythonscad-package.rpm:ro" \
    -v "$smoke_dir:/tmp/pythonscad-smoke" \
    -v "$SCRIPT_DIR:/release-smoke:ro" \
    -e PACKAGE_FILE="$package_file" \
    "$image" \
    bash -c '
      set -euo pipefail
      if command -v dnf >/dev/null 2>&1; then
        dnf install -y --nogpgcheck /tmp/pythonscad-package.rpm
      else
        yum install -y --nogpgcheck /tmp/pythonscad-package.rpm
      fi
      QT_QPA_PLATFORM=xcb xvfb-run -a bash -c '"'"'
        source /release-smoke/common-smoke.sh
        rs_smoke_binary /usr/bin/pythonscad "$PACKAGE_FILE" /tmp/pythonscad-smoke
      '"'"'
    '
}

for package in "${packages[@]}"; do
  package=$(realpath "$package")
  image=$(package_container_image "$package")
  smoke_dir="$workdir/package-smoke-$(basename "$package")"
  mkdir -p "$smoke_dir"

  rs_log "Smoke testing $(basename "$package") in $image"
  case "$package" in
    *.deb)
      run_deb_package_test "$package" "$image" "$smoke_dir"
      ;;
    *.rpm)
      run_rpm_package_test "$package" "$image" "$smoke_dir"
      ;;
    *)
      rs_die "unsupported package type: $package"
      ;;
  esac
done

rs_log "All Linux package smoke tests passed"
