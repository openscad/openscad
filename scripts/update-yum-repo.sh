#!/usr/bin/env bash
#
# Update YUM/DNF repository with new RPM packages
#
# This script maintains a YUM/DNF repository for PythonSCAD RPM packages.
# It should be run from the yum-repo branch.
#
# Usage:
#   ./update-yum-repo.sh /path/to/rpm/files
#
# Environment variables:
#   GPG_KEY          - GPG key ID to use for signing (optional)
#   GPG_PASSPHRASE   - GPG key passphrase (optional, for password-protected keys)
#   REPO_DIR         - Repository directory (default: current directory)
#

set -euo pipefail

# Color output
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly NC='\033[0m'

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

# Validate inputs
if [ $# -lt 1 ]; then
    die "Usage: $0 /path/to/rpm/files"
fi

RPM_SOURCE_DIR="$1"

if [ ! -d "$RPM_SOURCE_DIR" ]; then
    die "RPM source directory not found: $RPM_SOURCE_DIR"
fi

# Configuration
REPO_DIR="${REPO_DIR:-.}"
GPG_KEY="${GPG_KEY:-}"
GPG_PASSPHRASE="${GPG_PASSPHRASE:-}"
KEEP_VERSIONS="${KEEP_VERSIONS:-3}"
REPO_BASE_URL="${REPO_BASE_URL:-https://pythonscad-repos.nomike.org}"

info "PythonSCAD YUM Repository Updater"
info "================================="

# Check for required tools
if ! command_exists createrepo_c && ! command_exists createrepo; then
    die "createrepo not found. Install with: sudo dnf install createrepo_c"
fi

CREATEREPO_CMD="createrepo_c"
if ! command_exists createrepo_c; then
    CREATEREPO_CMD="createrepo"
fi

# Copy new packages to appropriate directories
info "Organizing packages..."

for rpm in "${RPM_SOURCE_DIR}"/*.rpm; do
    if [ ! -f "$rpm" ]; then
        warn "No RPM files found in ${RPM_SOURCE_DIR}"
        continue
    fi

    filename=$(basename "$rpm")

    # Determine architecture
    if [[ "$filename" == *"x86_64"* ]]; then
        arch="x86_64"
    elif [[ "$filename" == *"aarch64"* ]]; then
        arch="aarch64"
    elif [[ "$filename" == *"noarch"* ]]; then
        arch="x86_64"  # Put noarch in x86_64 directory
    else
        warn "Unknown architecture for $filename, skipping"
        continue
    fi

    # Dynamically extract distribution and version from dist tag
    # Examples: .fc43. -> fedora/43, .el9. -> el/9, .el10. -> el/10
    if [[ "$filename" =~ \.fc([0-9]+)\. ]]; then
        distro="fedora"
        version="${BASH_REMATCH[1]}"
        dest="${REPO_DIR}/packages/fedora/${version}/${arch}"
    elif [[ "$filename" =~ \.el([0-9]+)\. ]]; then
        distro="el"
        version="${BASH_REMATCH[1]}"
        dest="${REPO_DIR}/packages/el/${version}/${arch}"
    else
        warn "Could not determine distribution from filename: $filename, skipping"
        continue
    fi

    # Create directory if it doesn't exist
    mkdir -p "$dest"

    info "Copying $filename to $dest"
    cp "$rpm" "$dest/"
done

# Clean up old versions (keep last N versions per distro/arch)
if [ "$KEEP_VERSIONS" -gt 0 ]; then
    info "Cleaning up old versions (keeping last ${KEEP_VERSIONS} per distro/arch)..."

    for distro_dir in "${REPO_DIR}/packages"/*/*; do
        if [ ! -d "$distro_dir" ]; then
            continue
        fi

        for arch_dir in "$distro_dir"/*; do
            if [ ! -d "$arch_dir" ]; then
                continue
            fi

            # Find all pythonscad RPMs in this arch directory
            # Sort by modification time (newest first), skip first N, delete the rest
            OLD_PACKAGES=$(ls -t "$arch_dir"/pythonscad-*.rpm 2>/dev/null | tail -n +$((KEEP_VERSIONS + 1)) || true)

            if [ -n "$OLD_PACKAGES" ]; then
                echo "$OLD_PACKAGES" | while read -r pkg; do
                    info "  Removing old package: $(basename "$pkg")"
                    rm -f "$pkg"
                done
            fi
        done
    done
fi

# Create repository metadata for each directory
info "Generating repository metadata..."

for distro_dir in "${REPO_DIR}/packages"/*/*; do
    if [ ! -d "$distro_dir" ]; then
        continue
    fi

    for arch_dir in "$distro_dir"/*; do
        if [ ! -d "$arch_dir" ]; then
            continue
        fi

        # Check if there are any RPM files
        if ! ls "$arch_dir"/*.rpm >/dev/null 2>&1; then
            info "No packages in $arch_dir, skipping"
            continue
        fi

        info "Creating repository metadata in $arch_dir"

        if [ -d "$arch_dir/repodata" ]; then
            $CREATEREPO_CMD --update "$arch_dir"
        else
            $CREATEREPO_CMD "$arch_dir"
        fi

        # Sign repository metadata if GPG key is provided
        if [ -n "$GPG_KEY" ] && command_exists gpg; then
            info "Signing repository metadata in $arch_dir"

            GPG_OPTS="--batch --yes --pinentry-mode loopback --passphrase-fd 0"
            GPG_OPTS="$GPG_OPTS --detach-sign --armor --local-user $GPG_KEY"

            # Use passphrase from environment or empty string
            PASSPHRASE="${GPG_PASSPHRASE:-}"

            if echo "$PASSPHRASE" | gpg $GPG_OPTS "$arch_dir/repodata/repomd.xml"; then
                info "Successfully signed repomd.xml"
            else
                die "Failed to sign repomd.xml"
            fi
        fi
    done
done

# Export GPG public key (doesn't require passphrase)
if [ -n "$GPG_KEY" ] && command_exists gpg; then
    info "Exporting GPG public key..."
    gpg --batch --export --armor "$GPG_KEY" > "${REPO_DIR}/RPM-GPG-KEY-pythonscad"
    if [ -f "${REPO_DIR}/RPM-GPG-KEY-pythonscad" ]; then
        info "Public key exported to RPM-GPG-KEY-pythonscad"
    fi
fi

# Create .repo file for users
info "Creating repository configuration file..."

cat > "${REPO_DIR}/pythonscad.repo" <<EOF
# PythonSCAD YUM/DNF Repository
#
# Installation:
#   sudo curl -o /etc/yum.repos.d/pythonscad.repo \\
#     ${REPO_BASE_URL}/yum/pythonscad.repo
#   sudo dnf install pythonscad

[pythonscad]
name=PythonSCAD
baseurl=${REPO_BASE_URL}/yum/packages/fedora/\$releasever/\$basearch
enabled=1
gpgcheck=1
gpgkey=${REPO_BASE_URL}/yum/RPM-GPG-KEY-pythonscad
metadata_expire=1d

[pythonscad-el9]
name=PythonSCAD EL9
baseurl=${REPO_BASE_URL}/yum/packages/el/9/\$basearch
enabled=0
gpgcheck=1
gpgkey=${REPO_BASE_URL}/yum/RPM-GPG-KEY-pythonscad
metadata_expire=1d
EOF

# Create index.html
info "Creating repository index..."

# Dynamically discover available distributions and versions
FEDORA_VERSIONS=$(find "${REPO_DIR}/packages/fedora" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | sed 's|.*/||' | sort -n || true)
EL_VERSIONS=$(find "${REPO_DIR}/packages/el" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | sed 's|.*/||' | sort -n || true)

# Generate package download links
PACKAGE_LINKS=""
if [ -n "$FEDORA_VERSIONS" ]; then
    for ver in $FEDORA_VERSIONS; do
        PACKAGE_LINKS="${PACKAGE_LINKS}        <li><a href=\"packages/fedora/${ver}/\">Fedora ${ver} packages</a></li>\n"
    done
fi
if [ -n "$EL_VERSIONS" ]; then
    for ver in $EL_VERSIONS; do
        PACKAGE_LINKS="${PACKAGE_LINKS}        <li><a href=\"packages/el/${ver}/\">EL ${ver} packages</a></li>\n"
    done
fi

# Determine example Fedora versions for installation instructions
LATEST_FEDORA=$(echo "$FEDORA_VERSIONS" | tail -n1)
FEDORA_RANGE=""
if [ -n "$FEDORA_VERSIONS" ]; then
    FIRST_FEDORA=$(echo "$FEDORA_VERSIONS" | head -n1)
    if [ "$FIRST_FEDORA" = "$LATEST_FEDORA" ]; then
        FEDORA_RANGE="Fedora $LATEST_FEDORA"
    else
        FEDORA_RANGE="Fedora $FIRST_FEDORA-$LATEST_FEDORA"
    fi
fi

cat > "${REPO_DIR}/index.html" <<EOF
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PythonSCAD YUM Repository</title>
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
    <h1>PythonSCAD YUM/DNF Repository</h1>

    <p>This repository provides RPM packages for PythonSCAD on Fedora and RHEL-based distributions.</p>

    <h2>Installation</h2>

    <h3>${FEDORA_RANGE:-Fedora}</h3>
    <pre>sudo curl -o /etc/yum.repos.d/pythonscad.repo \\
  ${REPO_BASE_URL}/yum/pythonscad.repo
sudo dnf install pythonscad</pre>

    <h3>RHEL/Rocky/AlmaLinux</h3>
    <pre>sudo curl -o /etc/yum.repos.d/pythonscad.repo \\
  ${REPO_BASE_URL}/yum/pythonscad.repo
sudo dnf config-manager --set-enabled pythonscad-el9
sudo dnf install pythonscad</pre>

    <h2>Supported Architectures</h2>
    <ul>
        <li>x86_64 (AMD64)</li>
        <li>aarch64 (ARM64)</li>
    </ul>

    <h2>Manual Download</h2>
    <p>You can also download packages directly:</p>
    <ul>
$(echo -e "$PACKAGE_LINKS")
    </ul>

    <h2>GPG Key</h2>
    <p>Packages are signed with the PythonSCAD GPG key:</p>
    <pre>sudo rpm --import ${REPO_BASE_URL}/yum/RPM-GPG-KEY-pythonscad</pre>

    <h2>More Information</h2>
    <p>Visit <a href="https://github.com/pythonscad/pythonscad">github.com/pythonscad/pythonscad</a> for documentation and source code.</p>
</body>
</html>
EOF

info ""
info "Repository updated successfully!"
info ""
info "Repository structure:"
find "${REPO_DIR}/packages" -name "*.rpm" | head -10
info ""
info "To test locally:"
info "  sudo dnf config-manager --add-repo file://${PWD}/${REPO_DIR}/pythonscad.repo"
info "  sudo dnf install pythonscad"
