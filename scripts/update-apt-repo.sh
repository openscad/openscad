#!/usr/bin/env bash
#
# Update APT repository with new packages
#
# This script updates the APT repository structure with new .deb packages.
# It organizes packages by distribution codename and generates Packages indices
# for each distribution, then signs the Release files with GPG.
#
# Usage:
#   update-apt-repo.sh <packages_dir>
#
# Environment variables:
#   REPO_DIR         - Repository root directory (default: current directory)
#   GPG_KEY          - GPG key ID for signing (uses default key if not set)
#   GPG_PASSPHRASE   - GPG key passphrase (optional, for password-protected keys)
#   KEEP_VERSIONS    - Number of old versions to keep per architecture per distro (default: 3)
#   REPO_BASE_URL    - Base URL for the repository (used in HTML index)

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
GPG_PASSPHRASE="${GPG_PASSPHRASE:-}"
KEEP_VERSIONS="${KEEP_VERSIONS:-3}"
REPO_BASE_URL="${REPO_BASE_URL:-https://repo.pythonscad.org}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

info "APT Repository Updater"
info "======================"

# Validate packages directory
if [ ! -d "$PACKAGES_DIR" ]; then
    die "Packages directory not found: $PACKAGES_DIR"
fi

# Check for supported-distributions.json
if [ ! -f "$SCRIPT_DIR/supported-distributions.json" ]; then
    die "supported-distributions.json not found in $SCRIPT_DIR"
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

# Extract distribution codenames from supported-distributions.json
CODENAMES=$(python3 "$SCRIPT_DIR/scripts/extract-codenames.py" "$SCRIPT_DIR/supported-distributions.json")

# Create repository structure with separate pools per distribution
info "Creating repository structure with separate pools per distribution..."

# Copy new packages to distribution-specific pools
info "Organizing packages into distribution-specific pools..."
PACKAGE_COUNT=0

for deb in "$PACKAGES_DIR"/*.deb; do
    if [ ! -f "$deb" ]; then
        continue
    fi

    BASENAME=$(basename "$deb")

    # Extract distro from filename: pythonscad_VERSION-1_FAMILY_DISTRO_ARCH.deb
    # Regex captures the DISTRO field (3rd underscore-separated field)
    if [[ $BASENAME =~ pythonscad_[^_]+_[^_]+_([^_]+)_[^_]+\.deb ]]; then
        DISTRO="${BASH_REMATCH[1]}"
    else
        warn "Could not parse distro from filename: $BASENAME (skipping)"
        continue
    fi

    # Verify distro is in our supported list
    if ! echo "$CODENAMES" | grep -q "^${DISTRO}$"; then
        warn "Distribution '$DISTRO' not in supported-distributions.json (skipping: $BASENAME)"
        continue
    fi

    # Create pool directory for this specific distribution
    mkdir -p "pool/$DISTRO/main/p/pythonscad"
    cp "$deb" "pool/$DISTRO/main/p/pythonscad/$BASENAME"
    ((++PACKAGE_COUNT))
done

info "Total packages copied: $PACKAGE_COUNT"

# Create distribution directories and clean up old versions
info "Setting up distribution-specific directories..."

for DISTRO in $CODENAMES; do
    info "  Processing distribution: $DISTRO"

    mkdir -p "dists/$DISTRO/main/binary-amd64"
    mkdir -p "dists/$DISTRO/main/binary-arm64"

    # Clean up old versions (keep last N versions per architecture)
    if [ "$KEEP_VERSIONS" -gt 0 ] && [ -d "pool/$DISTRO/main/p/pythonscad" ]; then
        for arch in amd64 arm64; do
            # Find all packages for this distro and arch, sort by name, keep newest
            OLD_PACKAGES=$(ls -t "pool/$DISTRO/main/p/pythonscad"/*_*_${DISTRO}_${arch}.deb 2>/dev/null | tail -n +$((KEEP_VERSIONS + 1)) || true)
            if [ -n "$OLD_PACKAGES" ]; then
                while IFS= read -r pkg; do
                    info "    Removing old: $(basename "$pkg")"
                    rm -f "$pkg"
                done <<< "$OLD_PACKAGES"
            fi
        done
    fi
done

# Generate Packages files for each distribution and architecture
info "Generating Packages indices..."

for DISTRO in $CODENAMES; do
    for arch in amd64 arm64; do
        BINARY_DIR="dists/$DISTRO/main/binary-${arch}"

        # Check if there are packages for this distro/arch combination
        if [ -d "pool/$DISTRO/main/p/pythonscad" ] && ls "pool/$DISTRO/main/p/pythonscad"/*_*_${DISTRO}_${arch}.deb >/dev/null 2>&1; then
            info "  Generating Packages file for ${DISTRO}/${arch}..."

            cd "$BINARY_DIR"
            # Scan only packages in this distribution's pool
            dpkg-scanpackages --arch "$arch" "../../../../pool/$DISTRO/main/p/pythonscad" /dev/null > Packages
            gzip -k -f Packages

            # Calculate checksums for Release file
            md5sum Packages >> ../../../../.checksums_temp || true
            sha256sum Packages >> ../../../../.checksums_temp || true

            info "    $(wc -l < Packages) package entries for ${DISTRO}/${arch}"
            cd - > /dev/null
        else
            info "  No packages found for ${DISTRO}/${arch}"
        fi
    done

    # Generate Release file for this distribution
    info "  Generating Release file for ${DISTRO}..."

    cd "dists/$DISTRO"

    cat > Release <<EOF
Origin: PythonSCAD
Label: PythonSCAD
Suite: $DISTRO
Codename: $DISTRO
Architectures: amd64 arm64
Components: main
Description: PythonSCAD Official Repository - $DISTRO
Date: $(date -Ru)
EOF

    # Add file checksums to Release
    apt-ftparchive release . >> Release

    info "  Release file generated for $DISTRO"

    # Sign Release file with GPG
    info "  Signing Release file for ${DISTRO}..."

    GPG_OPTS="--batch --yes --pinentry-mode loopback"
    GPG_OPTS="$GPG_OPTS --passphrase-fd 0"

    if [ -n "$GPG_KEY" ]; then
        GPG_OPTS="$GPG_OPTS --local-user $GPG_KEY"
        info "    Using GPG key: $GPG_KEY"
    else
        info "    Using default GPG key"
    fi

    PASSPHRASE="${GPG_PASSPHRASE:-}"

    # Create detached signature
    if echo "$PASSPHRASE" | gpg $GPG_OPTS -abs -o Release.gpg Release; then
        info "    Created Release.gpg"
    else
        die "Failed to create detached signature for $DISTRO"
    fi

    # Create clearsigned file
    if echo "$PASSPHRASE" | gpg $GPG_OPTS --clearsign -o InRelease Release; then
        info "    Created InRelease"
    else
        die "Failed to create clearsigned InRelease for $DISTRO"
    fi

    cd - > /dev/null
done

# Export public GPG key for users (once, at repo root)
if [ ! -f pythonscad-archive-keyring.gpg ]; then
    info "Exporting public GPG key..."

    if [ -n "$GPG_KEY" ]; then
        gpg --batch --pinentry-mode loopback --armor --export "$GPG_KEY" > pythonscad-archive-keyring.gpg
    else
        DEFAULT_KEY=$(gpg --batch --list-secret-keys --keyid-format LONG 2>/dev/null | grep sec | head -n1 | awk '{print $2}' | cut -d'/' -f2 || true)
        if [ -n "$DEFAULT_KEY" ]; then
            gpg --batch --pinentry-mode loopback --armor --export "$DEFAULT_KEY" > pythonscad-archive-keyring.gpg
        else
            warn "No GPG key found, skipping keyring export"
        fi
    fi
fi

if [ -f pythonscad-archive-keyring.gpg ]; then
    info "Public key available at: pythonscad-archive-keyring.gpg"
fi

# Create index.html with distribution-specific instructions
info "Creating repository index..."

cat > index.html <<'HTMLEOF'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PythonSCAD APT Repository</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            max-width: 900px;
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
            border-left: 4px solid #3498db;
        }
        .warning {
            background: #fff3cd;
            border-left: 4px solid #ffc107;
            padding: 12px;
            margin: 20px 0;
        }
        .distro-list {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 15px;
            margin: 20px 0;
        }
        .distro-card {
            border: 1px solid #ddd;
            padding: 15px;
            border-radius: 5px;
            background: #fafafa;
        }
        .distro-card h3 {
            margin-top: 0;
            color: #2c3e50;
        }
    </style>
</head>
<body>
    <h1>🎉 PythonSCAD APT Repository</h1>

    <p>This repository provides Debian packages for PythonSCAD on Debian and Ubuntu-based distributions.</p>

    <h2>Quick Setup (Automatic Distro Detection)</h2>

    <p>Copy and paste this command to automatically detect your distribution and add the repository:</p>

    <pre><code>wget -qO - REPO_BASE_URL/apt/pythonscad-archive-keyring.gpg | \
  sudo gpg --dearmor -o /usr/share/keyrings/pythonscad-archive-keyring.gpg

echo "deb [signed-by=/usr/share/keyrings/pythonscad-archive-keyring.gpg] REPO_BASE_URL/apt $(lsb_release -sc) main" | \
  sudo tee /etc/apt/sources.list.d/pythonscad.list

sudo apt update
sudo apt install pythonscad</code></pre>

    <p><strong>Note:</strong> The command above uses <code>$(lsb_release -sc)</code> to automatically detect your distribution codename. If <code>lsb_release</code> is not available, you can manually replace <code>$(lsb_release -sc)</code> with your distribution codename (see list below).</p>

    <h2>Supported Distributions</h2>

    <p>This repository provides packages for the following distributions:</p>

    <div class="distro-list">
        <div class="distro-card">
            <h3>Ubuntu 22.04 LTS</h3>
            <p><strong>Codename:</strong> <code>jammy</code></p>
            <p>Long-term support until April 2027</p>
        </div>
        <div class="distro-card">
            <h3>Ubuntu 24.04 LTS</h3>
            <p><strong>Codename:</strong> <code>noble</code></p>
            <p>Long-term support until April 2029</p>
        </div>
        <div class="distro-card">
            <h3>Ubuntu 24.10</h3>
            <p><strong>Codename:</strong> <code>oracular</code></p>
            <p>Support until July 2025</p>
        </div>
        <div class="distro-card">
            <h3>Ubuntu 25.10</h3>
            <p><strong>Codename:</strong> <code>questing</code></p>
            <p>Support until July 2026</p>
        </div>
        <div class="distro-card">
            <h3>Debian 11 (Bullseye)</h3>
            <p><strong>Codename:</strong> <code>bullseye</code></p>
            <p>Long-term support until August 2026</p>
        </div>
        <div class="distro-card">
            <h3>Debian 12 (Bookworm)</h3>
            <p><strong>Codename:</strong> <code>bookworm</code></p>
            <p>Long-term support until June 2028</p>
        </div>
        <div class="distro-card">
            <h3>Debian 13 (Trixie)</h3>
            <p><strong>Codename:</strong> <code>trixie</code></p>
            <p>Testing distribution, support until June 2025</p>
        </div>
    </div>

    <h2>Check Your Distribution</h2>

    <p>To find your distribution codename, run:</p>
    <pre><code>lsb_release -sc</code></pre>

    <h2>Supported Architectures</h2>
    <ul>
        <li><strong>amd64</strong> - Intel/AMD 64-bit processors</li>
        <li><strong>arm64</strong> - ARM 64-bit processors (including Apple Silicon, Raspberry Pi 4/5, etc.)</li>
    </ul>

    <h2>Manual Package Download</h2>

    <p>Alternatively, you can download packages directly by distribution:</p>
    <ul>
        <li><a href="pool/">Browse packages by distribution</a></li>
        <li><a href="dists/">Repository metadata by distribution</a></li>
    </ul>

    <p>Package pools by distribution:</p>
    <ul>
        <li><a href="pool/jammy/main/p/pythonscad/">Ubuntu 22.04 (jammy)</a></li>
        <li><a href="pool/noble/main/p/pythonscad/">Ubuntu 24.04 (noble)</a></li>
        <li><a href="pool/oracular/main/p/pythonscad/">Ubuntu 24.10 (oracular)</a></li>
        <li><a href="pool/questing/main/p/pythonscad/">Ubuntu 25.10 (questing)</a></li>
        <li><a href="pool/bullseye/main/p/pythonscad/">Debian 11 (bullseye)</a></li>
        <li><a href="pool/bookworm/main/p/pythonscad/">Debian 12 (bookworm)</a></li>
        <li><a href="pool/trixie/main/p/pythonscad/">Debian 13 (trixie)</a></li>
    </ul>

    <h2>GPG Key Information</h2>

    <p>Packages are signed with the PythonSCAD GPG key for security. The key is automatically imported in the setup command above.</p>

    <p>To manually import the key:</p>
    <pre><code>wget -qO - REPO_BASE_URL/apt/pythonscad-archive-keyring.gpg | \
  sudo gpg --dearmor -o /usr/share/keyrings/pythonscad-archive-keyring.gpg</code></pre>

    <h2>Troubleshooting</h2>

    <h3>Command not found: lsb_release</h3>
    <p>If <code>lsb_release</code> is not available, you can:</p>
    <ol>
        <li>Install it: <code>sudo apt install lsb-release</code></li>
        <li>Or manually replace <code>$(lsb_release -sc)</code> with your codename from the list above</li>
        <li>Or check <code>/etc/os-release</code>: <code>grep VERSION_CODENAME /etc/os-release</code></li>
    </ol>

    <h3>Package dependencies not satisfied</h3>
    <p>If you see dependency errors, ensure you have the latest package list:</p>
    <pre><code>sudo apt update
sudo apt install -f</code></pre>

    <h2>More Information</h2>
    <ul>
        <li><a href="https://github.com/pythonscad/pythonscad">PythonSCAD on GitHub</a></li>
        <li><a href="https://pythonscad.org">PythonSCAD Website</a></li>
    </ul>
</body>
</html>
HTMLEOF

# Replace placeholder with actual URL
sed -i "s|REPO_BASE_URL|$REPO_BASE_URL|g" index.html

# Summary
info ""
info "Repository update complete!"
info ""
info "Repository structure:"
find dists/ -type f | head -20
info ""
info "Package statistics:"
for DISTRO in $CODENAMES; do
    for arch in amd64 arm64; do
        if [ -d "pool/$DISTRO/main/p/pythonscad" ]; then
            COUNT=$(ls "pool/$DISTRO/main/p/pythonscad"/*_*_${DISTRO}_${arch}.deb 2>/dev/null | wc -l) || COUNT=0
        else
            COUNT=0
        fi
        if [ "$COUNT" -gt 0 ]; then
            info "  $DISTRO/$arch: $COUNT packages"
        fi
    done
done
# Ensure we have a successful exit code from the loop
true
info ""
info "To use this repository, users should run:"
info "  wget -qO - $REPO_BASE_URL/apt/pythonscad-archive-keyring.gpg | \\"
info "    sudo gpg --dearmor -o /usr/share/keyrings/pythonscad-archive-keyring.gpg"
info "  echo 'deb [signed-by=/usr/share/keyrings/pythonscad-archive-keyring.gpg] $REPO_BASE_URL/apt \$(lsb_release -sc) main' | \\"
info "    sudo tee /etc/apt/sources.list.d/pythonscad.list"
info "  sudo apt update && sudo apt install pythonscad"
