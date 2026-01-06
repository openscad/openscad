#!/usr/bin/env bash
#
# Update APT repository with new packages
#
# This script updates the APT repository structure with new .deb packages.
# It generates the Packages indices and signs the Release file with GPG.
#
# Usage:
#   update-apt-repo.sh <packages_dir>
#
# Environment variables:
#   REPO_DIR         - Repository root directory (default: current directory)
#   GPG_KEY          - GPG key ID for signing (uses default key if not set)
#   KEEP_VERSIONS    - Number of old versions to keep per architecture (default: 3)
#

set -euo pipefail

# Color output
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly NC='\033[0m'

info() { echo -e "${GREEN}[INFO]${NC} $*"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }
die() { error "$*"; exit 1; }

command_exists() { command -v "$1" >/dev/null 2>&1; }

# Configuration
PACKAGES_DIR="${1:?Missing packages directory argument}"
REPO_DIR="${REPO_DIR:-.}"
GPG_KEY="${GPG_KEY:-}"
KEEP_VERSIONS="${KEEP_VERSIONS:-3}"
REPO_BASE_URL="${REPO_BASE_URL:-https://pythonscad-repos.nomike.org}"

info "APT Repository Updater"
info "======================"

# Validate packages directory
if [ ! -d "$PACKAGES_DIR" ]; then
    die "Packages directory not found: $PACKAGES_DIR"
fi

# Check for required tools
if ! command_exists dpkg-scanpackages; then
    die "dpkg-scanpackages not found. Please install: sudo apt-get install dpkg-dev"
fi

if ! command_exists apt-ftparchive; then
    die "apt-ftparchive not found. Please install: sudo apt-get install apt-utils"
fi

if ! command_exists gpg; then
    die "gpg not found. Please install: sudo apt-get install gnupg"
fi

cd "$REPO_DIR"

info "Repository directory: $(pwd)"
info "Packages directory: $PACKAGES_DIR"

# Create repository structure
info "Creating repository structure..."
mkdir -p pool/main/p/pythonscad
mkdir -p dists/stable/main/binary-amd64
mkdir -p dists/stable/main/binary-arm64

# Copy new packages to pool
info "Copying packages to pool..."
find "$PACKAGES_DIR" -name "*.deb" -exec cp -v {} pool/main/p/pythonscad/ \;

# Count packages
PACKAGE_COUNT=$(find pool/main/p/pythonscad -name "*.deb" | wc -l)
info "Total packages in pool: $PACKAGE_COUNT"

# Clean up old versions (keep last N versions per architecture)
if [ "$KEEP_VERSIONS" -gt 0 ]; then
    info "Cleaning up old versions (keeping last ${KEEP_VERSIONS} per architecture)..."

    for arch in amd64 arm64; do
        OLD_PACKAGES=$(ls -t pool/main/p/pythonscad/pythonscad_*_${arch}.deb 2>/dev/null | tail -n +$((KEEP_VERSIONS + 1)) || true)
        if [ -n "$OLD_PACKAGES" ]; then
            echo "$OLD_PACKAGES" | while read -r pkg; do
                info "  Removing old package: $(basename "$pkg")"
                rm -f "$pkg"
            done
        fi
    done
fi

# Generate Packages files for each architecture
info "Generating Packages indices..."

for arch in amd64 arm64; do
    BINARY_DIR="dists/stable/main/binary-${arch}"

    if find pool/main/p/pythonscad -name "*_${arch}.deb" | grep -q .; then
        info "  Generating Packages file for ${arch}..."
        cd "$BINARY_DIR"
        dpkg-scanpackages --arch "$arch" ../../../../pool/main/p/pythonscad /dev/null > Packages
        gzip -k -f Packages
        info "    $(wc -l < Packages) package entries for ${arch}"
        cd - > /dev/null
    else
        warn "  No packages found for ${arch}, skipping"
    fi
done

# Generate Release file
info "Generating Release file..."

cd dists/stable

cat > Release <<EOF
Origin: PythonSCAD
Label: PythonSCAD
Suite: stable
Codename: stable
Architectures: amd64 arm64
Components: main
Description: PythonSCAD Official Repository
Date: $(date -Ru)
EOF

# Add file checksums to Release
apt-ftparchive release . >> Release

info "Release file generated"

# Sign Release file with GPG
info "Signing Release file..."

GPG_OPTS="--batch --yes --pinentry-mode loopback"
if [ -n "$GPG_KEY" ]; then
    GPG_OPTS="$GPG_OPTS --local-user $GPG_KEY"
    info "Using GPG key: $GPG_KEY"
else
    info "Using default GPG key"
fi

# Create detached signature
if gpg $GPG_OPTS -abs -o Release.gpg Release; then
    info "Created Release.gpg (detached signature)"
else
    die "Failed to create detached signature"
fi

# Create clearsigned file
if gpg $GPG_OPTS --clearsign -o InRelease Release; then
    info "Created InRelease (clearsigned)"
else
    die "Failed to create clearsigned InRelease"
fi

cd - > /dev/null

# Export public GPG key for users
info "Exporting public GPG key..."
if [ -n "$GPG_KEY" ]; then
    gpg --batch --pinentry-mode loopback --armor --export "$GPG_KEY" > pythonscad-archive-keyring.gpg
else
    # Export default key
    DEFAULT_KEY=$(gpg --batch --list-secret-keys --keyid-format LONG | grep sec | head -n1 | awk '{print $2}' | cut -d'/' -f2)
    if [ -n "$DEFAULT_KEY" ]; then
        gpg --batch --pinentry-mode loopback --armor --export "$DEFAULT_KEY" > pythonscad-archive-keyring.gpg
    else
        warn "No GPG key found, skipping keyring export"
    fi
fi

if [ -f pythonscad-archive-keyring.gpg ]; then
    info "Public key exported to pythonscad-archive-keyring.gpg"
fi

# Create index.html
info "Creating repository index..."

cat > index.html <<EOF
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PythonSCAD APT Repository</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            max-width: 800px;
            margin: 40px auto;
            padding: 0 20px;
            line-height: 1.6;
            color: #333;
        }
        h1 { color: #2c3e50; }
        h2 { color: #34495e; margin-top: 30px; }
        code {
            background: #f4f4f4;
            padding: 2px 6px;
            border-radius: 3px;
            font-family: 'Courier New', monospace;
        }
        pre {
            background: #f4f4f4;
            padding: 15px;
            border-radius: 5px;
            overflow-x: auto;
        }
        .warning {
            background: #fff3cd;
            border-left: 4px solid #ffc107;
            padding: 12px;
            margin: 20px 0;
        }
    </style>
</head>
<body>
    <h1>PythonSCAD APT Repository</h1>

    <p>This repository provides Debian packages for PythonSCAD on Debian and Ubuntu-based distributions.</p>

    <h2>Installation</h2>

    <h3>Debian/Ubuntu</h3>
    <pre># Download and install GPG key
wget -qO - ${REPO_BASE_URL}/apt/pythonscad-archive-keyring.gpg | \\
  sudo gpg --dearmor -o /usr/share/keyrings/pythonscad-archive-keyring.gpg

# Add repository
echo "deb [signed-by=/usr/share/keyrings/pythonscad-archive-keyring.gpg] ${REPO_BASE_URL}/apt stable main" | \\
  sudo tee /etc/apt/sources.list.d/pythonscad.list

# Install
sudo apt update
sudo apt install pythonscad</pre>

    <h2>Supported Architectures</h2>
    <ul>
        <li>amd64 (x86_64)</li>
        <li>arm64 (aarch64)</li>
    </ul>

    <h2>Manual Download</h2>
    <p>You can also download packages directly:</p>
    <ul>
        <li><a href="pool/main/p/pythonscad/">All packages</a></li>
        <li><a href="dists/stable/">Repository metadata</a></li>
    </ul>

    <h2>GPG Key</h2>
    <p>Packages are signed with the PythonSCAD GPG key:</p>
    <pre>wget -qO - ${REPO_BASE_URL}/apt/pythonscad-archive-keyring.gpg | \\
  sudo gpg --dearmor -o /usr/share/keyrings/pythonscad-archive-keyring.gpg</pre>

    <h2>More Information</h2>
    <p>Visit <a href="https://github.com/pythonscad/pythonscad">github.com/pythonscad/pythonscad</a> for documentation and source code.</p>
</body>
</html>
EOF

# Summary
info ""
info "Repository update complete!"
info ""
info "Repository structure:"
tree -L 3 -I '*.deb' dists/ 2>/dev/null || find dists/ -type f | head -20
info ""
info "To use this repository, users should:"
info "  1. wget -qO - https://YOUR_DOMAIN/pythonscad-archive-keyring.gpg | sudo gpg --dearmor -o /usr/share/keyrings/pythonscad-archive-keyring.gpg"
info "  2. echo 'deb [signed-by=/usr/share/keyrings/pythonscad-archive-keyring.gpg] https://YOUR_DOMAIN stable main' | sudo tee /etc/apt/sources.list.d/pythonscad.list"
info "  3. sudo apt update && sudo apt install pythonscad"
