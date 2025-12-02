#!/bin/bash
# PythonSCAD Development Environment Setup Script
#
# This script sets up your local development environment with all necessary
# tools and configurations for contributing to PythonSCAD.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "=== PythonSCAD Development Environment Setup ==="
echo

# Check if we're in a git repository
if ! git rev-parse --git-dir > /dev/null 2>&1; then
    echo "Error: Not in a git repository"
    exit 1
fi

# Install pre-commit hooks
echo "Installing pre-commit hooks..."
if ! command -v pre-commit &> /dev/null; then
    echo "pre-commit is not installed. Installing..."
    if command -v pip3 &> /dev/null; then
        pip3 install pre-commit
    elif command -v pip &> /dev/null; then
        pip install pre-commit
    else
        echo "Error: pip is not available. Please install Python and pip first."
        exit 1
    fi
fi

cd "$PROJECT_ROOT"
pre-commit install --hook-type commit-msg --hook-type pre-commit
echo "✓ Pre-commit hooks installed"
echo

# Set up git commit message template
echo "Setting up git commit message template..."
git config commit.template .gitmessage
echo "✓ Commit message template configured"
echo

# Optional: Install commitlint locally for manual validation
read -p "Do you want to install commitlint locally (requires Node.js)? [y/N] " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    if command -v npm &> /dev/null; then
        echo "Installing commitlint..."
        npm install --save-dev @commitlint/config-conventional@19 @commitlint/cli@19
        echo "✓ Commitlint installed"
        echo
        echo "You can now validate commit messages with:"
        echo "  echo 'feat: my feature' | npx commitlint"
    else
        echo "Warning: npm is not available. Skipping commitlint installation."
        echo "Commit message validation will still work via pre-commit hooks."
    fi
fi
echo

echo "=== Setup Complete ==="
echo
echo "Your development environment is now configured!"
echo
echo "Next steps:"
echo "  1. Install build dependencies for your platform:"
echo "     - Linux/BSD: ./scripts/uni-get-dependencies.py --yes --profile pythonscad-qt5"
echo "     - macOS: ./scripts/macosx-build-dependencies.sh"
echo "     - Windows: ./scripts/msys2-install-dependencies.sh"
echo
echo "  2. Build the project following instructions in README.md"
echo
echo "  3. Make commits using conventional commit format:"
echo "     git commit -m 'feat: add new feature'"
echo "     git commit -m 'fix: resolve bug'"
echo
echo "  4. See CONTRIBUTING.md for detailed contribution guidelines"
echo
