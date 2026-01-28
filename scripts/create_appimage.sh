#!/usr/bin/env bash
#
# Create AppImage for PythonSCAD
#
# This script builds PythonSCAD and packages it as an AppImage.
# Requirements:
#   - linuxdeploy (for bundling Qt dependencies)
#   - appimagetool (for creating the final AppImage)
#   - linuxdeploy-plugin-qt (downloaded automatically if needed)
#   - cmake, make, and build dependencies for PythonSCAD
#
# Note: This script uses appimagetool instead of the deprecated linuxdeploy-plugin-python.
# Python runtime is bundled manually for better control and compatibility.
#

set -euo pipefail  # Exit on error, undefined variables, and pipe failures

# Color output for better readability
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
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

# Configuration
BUILD_DIR="${PROJECT_ROOT}/build"
DIST_DIR="${PROJECT_ROOT}/dist"
TOOLS_DIR="${DIST_DIR}/tools"
APPDIR="${DIST_DIR}/AppDir"
NUM_JOBS="${NUM_JOBS:-$(nproc)}"
QT_VERSION="${QT_VERSION:-6}"  # Default to Qt6, can be overridden with QT_VERSION=5

# Auto-detect architecture if not specified
if [ -z "${ARCH:-}" ]; then
    ARCH=$(uname -m)
    info "Auto-detected architecture: ${ARCH}"
else
    info "Using architecture from environment: ${ARCH}"
fi

# Auto-detect Python version
info "Detecting Python version..."
PYTHON_VERSION=$(python3 -c "import sys; v=sys.version_info; print(f'{v.major}.{v.minor}')")
PYTHON_VERSION_FULL=$(python3 -c "import sys; v=sys.version_info; print(f'{v.major}.{v.minor}.{v.micro}')")
info "Found Python ${PYTHON_VERSION_FULL}"

# Check for required tools
info "Checking for required tools..."

if ! command_exists cmake; then
    die "cmake not found. Please install cmake."
fi

if ! command_exists make; then
    die "make not found. Please install make."
fi

if ! command_exists linuxdeploy; then
    warn "linuxdeploy not found, will download it..."
    mkdir -p "${TOOLS_DIR}"
    LINUXDEPLOY_PATH="${TOOLS_DIR}/linuxdeploy-${ARCH}.AppImage"

    if [ ! -f "${LINUXDEPLOY_PATH}" ]; then
        info "Downloading linuxdeploy for ${ARCH}..."
        wget -O "${LINUXDEPLOY_PATH}" \
            "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${ARCH}.AppImage" \
            || die "Failed to download linuxdeploy"
        chmod +x "${LINUXDEPLOY_PATH}"
    fi
    # Make tools directory available in PATH
    export PATH="${TOOLS_DIR}:${PATH}"
else
    info "Found linuxdeploy"
fi

# Check for appimagetool (replacement for linuxdeploy for final packaging)
if ! command_exists appimagetool; then
    warn "appimagetool not found, will download it..."
    mkdir -p "${TOOLS_DIR}"
    APPIMAGETOOL_PATH="${TOOLS_DIR}/appimagetool-${ARCH}.AppImage"

    if [ ! -f "${APPIMAGETOOL_PATH}" ]; then
        info "Downloading appimagetool for ${ARCH}..."
        wget -O "${APPIMAGETOOL_PATH}" \
            "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-${ARCH}.AppImage" \
            || die "Failed to download appimagetool"
        chmod +x "${APPIMAGETOOL_PATH}"
    fi
    # Make tools directory available in PATH
    export PATH="${TOOLS_DIR}:${PATH}"
else
    info "Found appimagetool"
fi

# Check for linuxdeploy-plugin-qt
PLUGIN_QT_PATH=""
if command_exists linuxdeploy-plugin-qt; then
    PLUGIN_QT_PATH=$(command -v linuxdeploy-plugin-qt)
    info "Found linuxdeploy-plugin-qt at: ${PLUGIN_QT_PATH}"
else
    warn "linuxdeploy-plugin-qt not found, will download it..."
    mkdir -p "${TOOLS_DIR}"
    PLUGIN_QT_PATH="${TOOLS_DIR}/linuxdeploy-plugin-qt-${ARCH}.AppImage"

    if [ ! -f "${PLUGIN_QT_PATH}" ]; then
        info "Downloading linuxdeploy-plugin-qt for ${ARCH}..."
        wget -O "${PLUGIN_QT_PATH}" \
            "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${ARCH}.AppImage" \
            || die "Failed to download linuxdeploy-plugin-qt"
        chmod +x "${PLUGIN_QT_PATH}"
    fi
fi

# Ensure we're in the project root
cd "${PROJECT_ROOT}"

# Create dist directory structure
info "Setting up dist directory structure..."
mkdir -p "${DIST_DIR}"

# Clean up previous AppImage build artifacts
info "Cleaning up previous AppImage build artifacts..."
rm -rf "${APPDIR}"
# Remove old AppImage files from dist directory
rm -f "${DIST_DIR}"/*.AppImage

# Create build directory if it doesn't exist
if [ ! -d "${BUILD_DIR}" ]; then
    info "Creating build directory..."
    mkdir -p "${BUILD_DIR}"
fi

# Configure with CMake
info "Configuring with CMake for Qt${QT_VERSION}..."
cd "${BUILD_DIR}"

# Determine USE_QT6 flag based on QT_VERSION
if [ "${QT_VERSION}" = "6" ]; then
    USE_QT6_FLAG=ON
else
    USE_QT6_FLAG=OFF
fi

cmake .. \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DEXPERIMENTAL=ON \
    -DENABLE_PYTHON=ON \
    -DPYTHON_VERSION="${PYTHON_VERSION}" \
    -DENABLE_LIBFIVE=ON \
    -DUSE_QT6="${USE_QT6_FLAG}" \
    -DPORTABLE_BINARY=ON \
    || die "CMake configuration failed"

# Build
info "Building PythonSCAD (using ${NUM_JOBS} jobs)..."
make -j"${NUM_JOBS}" || die "Build failed"

# Install to AppDir
info "Installing to AppDir..."
make install DESTDIR="${APPDIR}" || die "Installation to AppDir failed"

# Return to project root
cd "${PROJECT_ROOT}"

# Fix paths: Move /usr/local/* to /usr/* for proper AppDir structure
if [ -d "${APPDIR}/usr/local" ]; then
    info "Relocating files from /usr/local to /usr..."
    # Merge /usr/local into /usr
    cp -r "${APPDIR}/usr/local/"* "${APPDIR}/usr/" || true
    rm -rf "${APPDIR}/usr/local"
fi

# Set up environment for linuxdeploy
# Request specific Qt plugins to be bundled
# Note: linuxdeploy-plugin-qt uses semicolons as delimiters
export EXTRA_QT_PLUGINS="svg;platforms"

# Configure Qt paths for linuxdeploy-plugin-qt
if [ "${QT_VERSION}" = "6" ]; then
    if command -v qmake6 >/dev/null 2>&1; then
        export QMAKE=$(command -v qmake6)
        QT_PLUGIN_PATH=$(qmake6 -query QT_INSTALL_PLUGINS)
        export QT_PLUGIN_PATH
        info "Using Qt6 from: $(qmake6 -query QT_INSTALL_PREFIX)"
        info "Qt plugins at: ${QT_PLUGIN_PATH}"
    else
        warn "qmake6 not found, linuxdeploy may not find Qt plugins correctly"
    fi
else
    if command -v qmake >/dev/null 2>&1; then
        export QMAKE=$(command -v qmake)
        QT_PLUGIN_PATH=$(qmake -query QT_INSTALL_PLUGINS)
        export QT_PLUGIN_PATH
        info "Using Qt5 from: $(qmake -query QT_INSTALL_PREFIX)"
        info "Qt plugins at: ${QT_PLUGIN_PATH}"
    else
        warn "qmake not found, linuxdeploy may not find Qt plugins correctly"
    fi
fi

# Add tools directory to PATH so linuxdeploy can find downloaded plugins
export PATH="${TOOLS_DIR}:${PATH}"

# Set the output version (you can override with OPENSCAD_VERSION environment variable)
if [ -n "${OPENSCAD_VERSION:-}" ]; then
    BASE_VERSION="${OPENSCAD_VERSION}"
    info "Using base version from environment: ${BASE_VERSION}"
else
    # Use centralized version establishment script
    source "${PROJECT_ROOT}/scripts/establish_version.sh"
    BASE_VERSION=$(openscad_version)
    info "Using base version from establish_version.sh: ${BASE_VERSION}"
fi

# Append Qt version to the version string for unique filenames
# This ensures Qt5/Qt6 builds have different names
export LINUXDEPLOY_OUTPUT_VERSION="${BASE_VERSION}-qt${QT_VERSION}-${ARCH}"
info "AppImage version: ${LINUXDEPLOY_OUTPUT_VERSION}"

# Remove problematic python.o file if it exists
# This file can cause issues with AppImage creation
# Use major.minor version for the path (e.g., python3.13)
PYTHON_O_DIR="${APPDIR}/usr/lib/python${PYTHON_VERSION}"
if [ -d "${PYTHON_O_DIR}" ]; then
    # Find and remove all python.o files in config directories
    find "${PYTHON_O_DIR}" -type f -name "python.o" -exec rm -f {} \; 2>/dev/null || true
    info "Cleaned up python.o files"
fi

# Set the desktop file path for linuxdeploy
DESKTOP_FILE="${APPDIR}/usr/share/applications/pythonscad.desktop"
if [ ! -f "${DESKTOP_FILE}" ]; then
    die "Desktop file not found at ${DESKTOP_FILE}"
fi
info "Using desktop file: ${DESKTOP_FILE}"

# Bundle dependencies with linuxdeploy
info "Bundling Qt dependencies with linuxdeploy..."

# Use linuxdeploy to bundle Qt libraries (NOT to create final AppImage)
# The --plugin qt handles Qt libraries and QML dependencies
# DEPLOY_PLATFORM_THEMES tells the plugin to deploy platform-related plugins
export DEPLOY_PLATFORM_THEMES=1

# Add AppDir/usr/lib to LD_LIBRARY_PATH so linuxdeploy can find custom libraries like libfive.so
export LD_LIBRARY_PATH="${APPDIR}/usr/lib:${LD_LIBRARY_PATH:-}"

if command_exists linuxdeploy; then
    linuxdeploy \
        --appdir "${APPDIR}" \
        --desktop-file "${DESKTOP_FILE}" \
        --plugin qt \
        || die "Failed to bundle Qt dependencies"
elif [ -f "${LINUXDEPLOY_PATH}" ]; then
    "${LINUXDEPLOY_PATH}" \
        --appdir "${APPDIR}" \
        --desktop-file "${DESKTOP_FILE}" \
        --plugin qt \
        || die "Failed to bundle Qt dependencies"
else
    die "linuxdeploy not found"
fi

# Bundle Python runtime and libraries
info "Bundling Python runtime..."

# The Python runtime should already be in AppDir from 'make install'
# But we need to ensure all dependencies are included
PYTHON_LIB_DIR="${APPDIR}/usr/lib/python${PYTHON_VERSION}"
if [ ! -d "${PYTHON_LIB_DIR}" ]; then
    warn "Python library directory not found at ${PYTHON_LIB_DIR}"
    info "Attempting to copy system Python libraries..."
    mkdir -p "${PYTHON_LIB_DIR}"

    # Copy essential Python standard library
    if [ -d "/usr/lib/python${PYTHON_VERSION}" ]; then
        cp -r "/usr/lib/python${PYTHON_VERSION}"/* "${PYTHON_LIB_DIR}/" || true
    fi
fi

# Ensure Python shared library is included
info "Copying Python shared libraries..."
PYTHON_SO="libpython${PYTHON_VERSION}.so"
if [ ! -f "${APPDIR}/usr/lib/${PYTHON_SO}"* ]; then
    # Find and copy the Python shared library
    find /usr/lib -name "${PYTHON_SO}*" -exec cp -P {} "${APPDIR}/usr/lib/" \; 2>/dev/null || true
fi

# Set up proper AppDir structure for appimagetool
info "Setting up AppDir structure..."

# Desktop file was already found earlier for linuxdeploy

# Find the main executable
MAIN_EXEC=$(find "${APPDIR}/usr/bin" -type f -executable | head -1)
if [ -z "${MAIN_EXEC}" ]; then
    die "No executable found in AppDir/usr/bin"
fi

info "Found executable: ${MAIN_EXEC}"
EXEC_NAME=$(basename "${MAIN_EXEC}")

# Create AppRun script
info "Creating AppRun..."

# Remove any existing AppRun (linuxdeploy may have created a symlink)
rm -f "${APPDIR}/AppRun"

cat > "${APPDIR}/AppRun" <<APPRUN_EOF
#!/bin/bash
# AppRun script for PythonSCAD

SELF=\$(readlink -f "\$0")
HERE=\${SELF%/*}

# Set up environment
export PATH="\${HERE}/usr/bin:\${PATH}"
export LD_LIBRARY_PATH="\${HERE}/usr/lib:\${LD_LIBRARY_PATH}"
export PYTHONPATH="\${HERE}/usr/lib/python${PYTHON_VERSION}:\${HERE}/usr/lib/python${PYTHON_VERSION}/site-packages:\${PYTHONPATH}"
export PYTHONHOME="\${HERE}/usr"
export QT_PLUGIN_PATH="\${HERE}/usr/plugins"

# Force X11 platform (xcb) for maximum compatibility
# PythonSCAD uses OpenGL (QOpenGLWidget) which works better with XWayland on Wayland systems
# than native Wayland support in AppImages (requires complex EGL integration bundling)
export QT_QPA_PLATFORM=xcb

# Run the application
exec "\${HERE}/usr/bin/${EXEC_NAME}" "\$@"
APPRUN_EOF

chmod +x "${APPDIR}/AppRun"

# Create symlinks in AppDir root
ln -sf "${DESKTOP_FILE#${APPDIR}/}" "${APPDIR}/$(basename "${DESKTOP_FILE}")"

# Find and symlink icon
ICON_NAME=$(grep "^Icon=" "${DESKTOP_FILE}" | cut -d= -f2)
ICON_FILE=$(find "${APPDIR}/usr/share" -name "${ICON_NAME}.*" -type f | head -1)
if [ -n "${ICON_FILE}" ]; then
    ln -sf "${ICON_FILE#${APPDIR}/}" "${APPDIR}/$(basename "${ICON_FILE}")"
    info "Created icon symlink: $(basename "${ICON_FILE}")"
else
    warn "Icon file not found for: ${ICON_NAME}"
fi

# Create the final AppImage
info "Creating AppImage with appimagetool..."

# Set version for the output filename
if [ -n "${LINUXDEPLOY_OUTPUT_VERSION:-}" ]; then
    export VERSION="${LINUXDEPLOY_OUTPUT_VERSION}"
fi

# Change to dist directory so appimagetool creates the AppImage there
cd "${DIST_DIR}"

# Use appimagetool to create the final AppImage
# Note: appimagetool uses the VERSION environment variable for the filename
if command_exists appimagetool; then
    appimagetool "${APPDIR}" || die "AppImage creation failed"
elif [ -f "${APPIMAGETOOL_PATH}" ]; then
    "${APPIMAGETOOL_PATH}" "${APPDIR}" || die "AppImage creation failed"
else
    die "appimagetool not found"
fi

# Return to project root
cd "${PROJECT_ROOT}"

# Report success
info "AppImage created successfully!"
echo ""

# List the created AppImage files in dist directory
cd "${DIST_DIR}"
APPIMAGES=(PythonSCAD*.AppImage OpenSCAD*.AppImage pythonscad*.AppImage)
for pattern in "${APPIMAGES[@]}"; do
    if compgen -G "${pattern}" > /dev/null; then
        for appimage in ${pattern}; do
            if [ -f "${appimage}" ]; then
                info "Created: dist/${appimage} ($(du -h "${appimage}" | cut -f1))"
            fi
        done
    fi
done
cd "${PROJECT_ROOT}"

echo ""
info "You can now run the AppImage directly:"
echo "  ./dist/PythonSCAD-*.AppImage"
echo ""
info "Or install it to your system:"
echo "  chmod +x dist/PythonSCAD-*.AppImage"
echo "  sudo mv dist/PythonSCAD-*.AppImage /usr/local/bin/pythonscad"
