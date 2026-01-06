#!/usr/bin/env bash
#
# Build Debian package for PythonSCAD
#
# This script builds a Debian .deb package for PythonSCAD.
# It can be run locally or in CI/CD environments.
#
# Requirements:
#   - debhelper (>= 13)
#   - dpkg-buildpackage
#   - Build dependencies listed in debian/control
#
# Environment variables:
#   SIGN_PACKAGE     - Set to 'yes' to sign the package with GPG (default: no)
#   GPG_KEY          - GPG key ID to use for signing (required if SIGN_PACKAGE=yes)
#   RUN_LINTIAN      - Set to 'no' to skip lintian checks (default: yes)
#   OUTPUT_DIR       - Directory for output files (default: ./dist/debian)
#   DEBIAN_DISTRO    - Target distribution for changelog (default: unstable)
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
SIGN_PACKAGE="${SIGN_PACKAGE:-no}"
GPG_KEY="${GPG_KEY:-}"
RUN_LINTIAN="${RUN_LINTIAN:-yes}"
OUTPUT_DIR="${OUTPUT_DIR:-${PROJECT_ROOT}/dist/debian}"
DEBIAN_DISTRO="${DEBIAN_DISTRO:-unstable}"

info "PythonSCAD Debian Package Builder"
info "=================================="

# Check for required tools
info "Checking for required tools..."

if ! command_exists dpkg-buildpackage; then
    die "dpkg-buildpackage not found. Please install: sudo apt-get install dpkg-dev"
fi

if ! command_exists dh; then
    die "debhelper not found. Please install: sudo apt-get install debhelper"
fi

if [ "$RUN_LINTIAN" = "yes" ] && ! command_exists lintian; then
    warn "lintian not found, quality checks will be skipped. Install with: sudo apt-get install lintian"
    RUN_LINTIAN="no"
fi

# Note: Individual .deb signing with dpkg-sig is deprecated
# Modern practice is to sign the APT repository Release file instead
# See scripts/update-apt-repo.sh for repository-level signing
if [ "$SIGN_PACKAGE" = "yes" ]; then
    warn "Individual .deb signing is deprecated - packages will be unsigned"
    warn "Use repository-level signing instead (handled by update-apt-repo.sh)"
    SIGN_PACKAGE="no"
fi

# Verify we're in the project root
if [ ! -f "${PROJECT_ROOT}/VERSION.txt" ]; then
    die "VERSION.txt not found in ${PROJECT_ROOT}. Are you in the correct directory?"
fi

if [ ! -d "${PROJECT_ROOT}/debian" ]; then
    die "debian/ directory not found in ${PROJECT_ROOT}. Have you created the package metadata?"
fi

# Get version from VERSION.txt
VERSION=$(cat "${PROJECT_ROOT}/VERSION.txt" | tr -d '\n' | tr -d '\r')
if [ -z "$VERSION" ]; then
    die "VERSION.txt is empty"
fi

info "Building version: ${VERSION}"

# Generate debian/changelog
info "Generating debian/changelog..."

CHANGELOG_DATE=$(date -R)

cat > "${PROJECT_ROOT}/debian/changelog" <<EOF
pythonscad (${VERSION}-1) ${DEBIAN_DISTRO}; urgency=medium

  * New upstream release ${VERSION}
  * See CHANGELOG.md for full details

 -- PythonSCAD Developers <noreply@pythonscad.org>  ${CHANGELOG_DATE}
EOF

info "Changelog generated for version ${VERSION}-1"

# Detect architecture
ARCH=$(dpkg --print-architecture)
info "Building for architecture: ${ARCH}"

# Build the package
info "Building Debian package..."
info "This may take several minutes depending on your system..."

cd "${PROJECT_ROOT}"

# Build options
BUILD_OPTS="-b"  # Binary-only build
BUILD_OPTS="${BUILD_OPTS} -us -uc"  # Don't sign (we'll sign later if needed)

# Set parallel build jobs
if command_exists nproc; then
    JOBS=$(nproc)
    export DEB_BUILD_OPTIONS="parallel=${JOBS}"
    info "Building with ${JOBS} parallel jobs"
fi

# Run dpkg-buildpackage
if dpkg-buildpackage ${BUILD_OPTS}; then
    info "Package built successfully"
else
    die "Package build failed"
fi

# The .deb file is created in the parent directory
DEB_FILE="../pythonscad_${VERSION}-1_${ARCH}.deb"
CHANGES_FILE="../pythonscad_${VERSION}-1_${ARCH}.changes"
BUILDINFO_FILE="../pythonscad_${VERSION}-1_${ARCH}.buildinfo"

if [ ! -f "$DEB_FILE" ]; then
    die "Expected .deb file not found: $DEB_FILE"
fi

info "Package created: $(basename "$DEB_FILE")"

# Run lintian checks
if [ "$RUN_LINTIAN" = "yes" ]; then
    info "Running lintian quality checks..."
    if lintian "$DEB_FILE" || true; then
        info "Lintian checks completed"
    fi
fi

# Individual package signing is deprecated
# Packages will be verified via repository-level GPG signing
# (Release file signature in the APT repository)

# Create output directory and copy files
info "Copying files to output directory: ${OUTPUT_DIR}"
mkdir -p "${OUTPUT_DIR}"

# Convert to absolute path
DEB_FILE_ABS="$(cd "$(dirname "$DEB_FILE")" && pwd)/$(basename "$DEB_FILE")"

cp "$DEB_FILE_ABS" "${OUTPUT_DIR}/"

# Package info
info "Package information:"
info "  File: $(basename "$DEB_FILE_ABS")"
info "  Size: $(du -h "$DEB_FILE_ABS" | cut -f1)"
info "  Location: ${OUTPUT_DIR}/$(basename "$DEB_FILE_ABS")"

# Show package contents summary
info "Package contents summary:"
dpkg-deb --info "$DEB_FILE_ABS" | grep -E "Package|Version|Architecture|Depends|Description" || true

info ""
info "Build complete!"
info ""
info "To install locally:"
info "  sudo dpkg -i ${OUTPUT_DIR}/$(basename "$DEB_FILE_ABS")"
info "  sudo apt-get install -f  # Fix any dependency issues"
info ""
info "To inspect package contents:"
info "  dpkg -c ${OUTPUT_DIR}/$(basename "$DEB_FILE_ABS")"
info ""
info "To get detailed package information:"
info "  dpkg-deb --info ${OUTPUT_DIR}/$(basename "$DEB_FILE_ABS")"
