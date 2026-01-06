#!/usr/bin/env bash
#
# Build RPM package for PythonSCAD
#
# This script builds an RPM package for PythonSCAD.
# It can be run locally or in CI/CD environments.
#
# Requirements:
#   - rpm-build
#   - rpmdevtools
#   - Build dependencies listed in pythonscad.spec
#
# Environment variables:
#   RUN_RPMLINT      - Set to 'no' to skip rpmlint checks (default: yes)
#   OUTPUT_DIR       - Directory for output files (default: ./dist/rpm)
#   FEDORA_RELEASE   - Target Fedora release for dist tag (default: auto-detect)
#

set -euo pipefail  # Exit on error, undefined variables, and pipe failures

# Color output for better readability
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m' # No Color

# Helper functions
info() {
    echo -e "${GREEN}[INFO]${NC} $*"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $*"
}

error() {
    echo -e "${RED}[ERROR]${NC} $*" >&2
}

die() {
    error "$*"
    exit 1
}

# Check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Configuration (can be overridden via environment variables)
RUN_RPMLINT="${RUN_RPMLINT:-yes}"
OUTPUT_DIR="${OUTPUT_DIR:-${PROJECT_ROOT}/dist/rpm}"
FEDORA_RELEASE="${FEDORA_RELEASE:-}"

info "PythonSCAD RPM Package Builder"
info "=============================="

# Check for required tools
info "Checking for required tools..."

if ! command_exists rpmbuild; then
    die "rpmbuild not found. Please install: sudo dnf install rpm-build rpmdevtools (Fedora/RHEL) or sudo apt-get install rpm (Ubuntu/Debian)"
fi

# Note: rpmdev-setuptree is part of rpmdevtools which is Fedora/RHEL specific
# On Ubuntu/Debian, this tool doesn't exist but we handle it by manually creating
# the directory structure below, so this is just an informational warning
if ! command_exists rpmdev-setuptree; then
    warn "rpmdev-setuptree not found (expected on Ubuntu/Debian, will create directories manually)"
fi

if [ "$RUN_RPMLINT" = "yes" ] && ! command_exists rpmlint; then
    warn "rpmlint not found, quality checks will be skipped. Install with: sudo dnf install rpmlint (Fedora/RHEL) or sudo apt-get install rpmlint (Ubuntu/Debian)"
    RUN_RPMLINT="no"
fi

# Verify we're in the project root
if [ ! -f "${PROJECT_ROOT}/VERSION.txt" ]; then
    die "VERSION.txt not found in ${PROJECT_ROOT}. Are you in the correct directory?"
fi

if [ ! -f "${PROJECT_ROOT}/pythonscad.spec" ]; then
    die "pythonscad.spec not found in ${PROJECT_ROOT}. Have you created the spec file?"
fi

# Get version from VERSION.txt
VERSION=$(cat "${PROJECT_ROOT}/VERSION.txt" | tr -d '\n' | tr -d '\r')
if [ -z "$VERSION" ]; then
    die "VERSION.txt is empty"
fi

info "Building version: ${VERSION}"

# Detect architecture
ARCH=$(uname -m)
info "Building for architecture: ${ARCH}"

# Export environment variables needed by spec file
# These must be set before dnf builddep parses the spec file
export VERSION
export CHANGELOG_DATE=$(date "+%a %b %d %Y")

# Install build dependencies from spec file
info "Installing build dependencies..."
if command_exists dnf; then
    info "Using dnf to install BuildRequires from spec file..."
    if dnf builddep -y "${PROJECT_ROOT}/pythonscad.spec"; then
        info "Build dependencies installed successfully"
    else
        warn "Failed to install some build dependencies, build may fail"
    fi
elif command_exists yum; then
    info "Using yum-builddep to install BuildRequires from spec file..."
    if yum-builddep -y "${PROJECT_ROOT}/pythonscad.spec"; then
        info "Build dependencies installed successfully"
    else
        warn "Failed to install some build dependencies, build may fail"
    fi
else
    warn "Neither dnf nor yum found, skipping automatic dependency installation"
    warn "You may need to manually install BuildRequires listed in pythonscad.spec"
fi

# Set up RPM build tree
info "Setting up RPM build tree..."
if command_exists rpmdev-setuptree; then
    rpmdev-setuptree
else
    mkdir -p ~/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
fi

RPMBUILD_DIR="${HOME}/rpmbuild"

# Create source tarball
info "Creating source tarball..."
TARBALL_NAME="pythonscad-${VERSION}.tar.gz"
TEMP_DIR=$(mktemp -d)

# Copy project to temp directory
cp -r "${PROJECT_ROOT}" "${TEMP_DIR}/pythonscad-${VERSION}"

# Clean up build artifacts and git metadata
cd "${TEMP_DIR}/pythonscad-${VERSION}"
rm -rf .git .github build dist obj-* debian *.deb *.changes *.buildinfo
rm -f deb-build.log deb-build-clean.log

# Create tarball
cd "${TEMP_DIR}"
tar czf "${TARBALL_NAME}" "pythonscad-${VERSION}"
cp "${TARBALL_NAME}" "${RPMBUILD_DIR}/SOURCES/"

# Clean up temp directory
rm -rf "${TEMP_DIR}"

info "Source tarball created: ${TARBALL_NAME}"

# Copy spec file to SPECS directory
cp "${PROJECT_ROOT}/pythonscad.spec" "${RPMBUILD_DIR}/SPECS/"

# Detect dist tag if not provided
if [ -z "$FEDORA_RELEASE" ]; then
    if [ -f /etc/fedora-release ]; then
        FEDORA_RELEASE=$(rpm -E %{fedora})
        DIST_TAG=".fc${FEDORA_RELEASE}"
    elif [ -f /etc/redhat-release ]; then
        RHEL_VERSION=$(rpm -E %{rhel})
        DIST_TAG=".el${RHEL_VERSION}"
    else
        # Default for Ubuntu/Debian or unknown systems
        # Using .local to indicate it's a locally built package
        DIST_TAG=".local"
        info "Building on non-Fedora/RHEL system, using .local dist tag"
    fi
else
    # FEDORA_RELEASE is set - determine if it's Fedora or EL
    # Check for DISTRO_TYPE environment variable first
    if [ -n "${DISTRO_TYPE}" ]; then
        if [ "${DISTRO_TYPE}" = "el" ]; then
            DIST_TAG=".el${FEDORA_RELEASE}"
        else
            DIST_TAG=".fc${FEDORA_RELEASE}"
        fi
    # Fall back to detecting from system files
    elif [ -f /etc/fedora-release ]; then
        DIST_TAG=".fc${FEDORA_RELEASE}"
    elif [ -f /etc/redhat-release ]; then
        DIST_TAG=".el${FEDORA_RELEASE}"
    else
        # Default to Fedora if we can't detect
        DIST_TAG=".fc${FEDORA_RELEASE}"
    fi
fi

info "Using dist tag: ${DIST_TAG}"

# Build the package
info "Building RPM package..."
info "This may take several minutes depending on your system..."

cd "${RPMBUILD_DIR}/SPECS"

# Build options
BUILD_OPTS="-bb"  # Binary package only

# Set parallel build jobs
if command_exists nproc; then
    JOBS=$(nproc)
    info "Building with ${JOBS} parallel jobs"
    # RPM build system uses _smp_mflags
fi

# Run rpmbuild
if rpmbuild ${BUILD_OPTS} \
    --define "dist ${DIST_TAG}" \
    --define "VERSION ${VERSION}" \
    --define "CHANGELOG_DATE ${CHANGELOG_DATE}" \
    pythonscad.spec; then
    info "Package built successfully"
else
    die "Package build failed"
fi

# Find the built RPM
RPM_FILE=$(find "${RPMBUILD_DIR}/RPMS" -name "pythonscad-${VERSION}-1*.rpm" -type f | head -1)

if [ ! -f "$RPM_FILE" ]; then
    die "Expected RPM file not found in ${RPMBUILD_DIR}/RPMS"
fi

info "Package created: $(basename "$RPM_FILE")"

# Run rpmlint checks
if [ "$RUN_RPMLINT" = "yes" ]; then
    info "Running rpmlint quality checks..."
    if rpmlint "$RPM_FILE" || true; then
        info "Rpmlint checks completed"
    fi
fi

# Create output directory and copy files
info "Copying files to output directory: ${OUTPUT_DIR}"
mkdir -p "${OUTPUT_DIR}"

RPM_FILE_ABS="$(realpath "$RPM_FILE")"
cp "$RPM_FILE_ABS" "${OUTPUT_DIR}/"

# Package info
info "Package information:"
info "  File: $(basename "$RPM_FILE_ABS")"
info "  Size: $(du -h "$RPM_FILE_ABS" | cut -f1)"
info "  Location: ${OUTPUT_DIR}/$(basename "$RPM_FILE_ABS")"

# Show package contents summary
info "Package metadata:"
rpm -qip "$RPM_FILE_ABS" | grep -E "Name|Version|Release|Architecture|Summary" || true

info ""
info "Build complete!"
info ""
info "To install locally:"
info "  sudo dnf install ${OUTPUT_DIR}/$(basename "$RPM_FILE_ABS")"
info "  # or"
info "  sudo rpm -ivh ${OUTPUT_DIR}/$(basename "$RPM_FILE_ABS")"
info ""
info "To inspect package contents:"
info "  rpm -qlp ${OUTPUT_DIR}/$(basename "$RPM_FILE_ABS")"
info ""
info "To get detailed package information:"
info "  rpm -qip ${OUTPUT_DIR}/$(basename "$RPM_FILE_ABS")"
