# Contributing to PythonSCAD

Thank you for your interest in contributing to PythonSCAD!
This document explains our development workflow, versioning
system, and contribution guidelines.

## Table of Contents

- [Development Setup](#development-setup)
- [Commit Message Guidelines](#commit-message-guidelines)
- [Pull Request Process](#pull-request-process)
- [Versioning and Releases](#versioning-and-releases)
- [Code Style](#code-style)

## Development Setup

### Prerequisites

PythonSCAD requires the following tools for development:

- **Git**: For version control
- **CMake**: For building the project
- **Python 3**: For build scripts and Python integration
- **Node.js** (optional): Only needed if you want to run
  commitlint locally
- **pre-commit**: For automated code quality checks

### Setting Up Your Environment

1. **Clone the repository**:

   ```bash
   git clone https://github.com/pythonscad/pythonscad.git
   cd pythonscad
   git submodule update --init --recursive
   ```

2. **Install dependencies**:

   The project provides scripts to install build dependencies
   for your platform:

   - **Linux/BSD**:
     `./scripts/get-dependencies.py --yes --profile pythonscad-qt5`
   - **macOS**: `./scripts/macosx-build-dependencies.sh`
   - **Windows (MSYS2)**:
     `./scripts/get-dependencies.py --yes --profile pythonscad-qt6`

3. **Install pre-commit hooks**:

   ```bash
   pip install pre-commit
   pre-commit install --hook-type commit-msg --hook-type pre-commit
   ```

   **Important**: Both `--hook-type` flags are required:
   - `--hook-type pre-commit`: Runs before commit (code
     formatting, trailing whitespace, YAML validation)
   - `--hook-type commit-msg`: Validates commit message
     format (prevents invalid commit types)

   This sets up automated checks that run:
   - **Before each commit**: code formatting (clang-format),
     trailing whitespace, YAML validation
   - **On commit messages**: conventional commit format
     validation (catches invalid types like "debug:",
     "update:", etc.)

4. **Build the project**:

   Follow the platform-specific build instructions in
   [README.md](README.md).

### Local Development Tools

The pre-commit hooks will automatically validate your commit
messages, but if you want to validate them manually:

```bash
# Install commitlint locally (optional)
npm install

# Validate a commit message
echo "feat: add new feature" | npx commitlint

# Validate the last commit
npx commitlint --from HEAD~1
```

## Commit Message Guidelines

PythonSCAD uses
[Conventional Commits](https://www.conventionalcommits.org/)
for automated versioning and changelog generation.

### Commit Message Format

Each commit message must follow this structure:

```text
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

### Commit Types

- **feat**: A new feature
  (triggers minor version bump: 0.6.0 → 0.7.0)
- **fix**: A bug fix
  (triggers patch version bump: 0.6.0 → 0.6.1)
- **docs**: Documentation only changes
- **style**: Changes that don't affect code meaning
  (whitespace, formatting)
- **refactor**: Code changes that neither fix bugs nor
  add features
- **perf**: Performance improvements
- **test**: Adding or correcting tests
- **build**: Changes to build system or dependencies
- **ci**: Changes to CI configuration files and scripts
- **chore**: Other changes that don't modify src or test files
- **revert**: Reverts a previous commit

### Breaking Changes

To indicate a breaking change
(triggers major version bump: 0.6.0 → 1.0.0):

**Option 1**: Add `!` after the type:

```text
feat!: remove deprecated API
```

**Option 2**: Add `BREAKING CHANGE:` in the footer:

```text
feat: redesign coordinate system

BREAKING CHANGE: The origin is now at the center instead
of bottom-left. This will affect all existing models.
```

### Examples

**Good commit messages:**

```text
feat: add GPU-accelerated rendering

fix: resolve memory leak in CGAL renderer

docs: update installation instructions for Ubuntu 24.04

perf: optimize mesh generation by 40%

feat(python): add support for numpy arrays in polygon()

fix(gui): prevent crash when opening invalid STL files
```

**Bad commit messages:**

```text
Fixed stuff              (no type, not descriptive)
feat added feature       (wrong format)
Updated code             (no type, not descriptive)
fix: Fixed the bug.      (ends with period, redundant "fixed")
```

### Why Conventional Commits?

1. **Automated versioning**: The type determines version
   bumps automatically
2. **Automatic changelog**: Commits are grouped into
   Features, Bug Fixes, etc.
3. **Clear history**: Easy to understand what each commit does
4. **No manual decisions**: No debates about version numbers

## Pull Request Process

### Creating a Pull Request

1. **Create a feature branch**:

   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes** with proper commit messages:

   ```bash
   git commit -m "feat: add awesome feature"
   ```

3. **Push to GitHub**:

   ```bash
   git push origin feature/your-feature-name
   ```

4. **Open a Pull Request** on GitHub against the `master`
   branch

### PR Requirements

- All commits must follow conventional commit format
- Pre-commit hooks must pass (code formatting, linting)
- CI tests must pass
- Code should be properly tested
- PR title should also follow conventional commit format

The PR title follows the same format because it becomes
the merge commit message.

### What Happens After Merge

When your PR is merged to `master`:

1. **Release Please bot** analyzes all commits since the
   last release
2. It creates or updates a **Release PR** with:
   - Updated `VERSION.txt`
   - Updated `CHANGELOG.md`
   - All changes since the last release
3. Maintainers review and merge the Release PR when ready
   to publish
4. A new version is tagged and released on GitHub

**Important**: Your feature goes to `master` immediately,
but the actual release happens separately when the Release
PR is merged.

## Versioning and Releases

### Semantic Versioning

PythonSCAD uses
[Semantic Versioning](https://semver.org/) (SemVer):

```text
MAJOR.MINOR.PATCH
```

- **MAJOR** (e.g., 1.0.0 → 2.0.0): Breaking changes
- **MINOR** (e.g., 0.6.0 → 0.7.0): New features
  (backward compatible)
- **PATCH** (e.g., 0.6.1 → 0.6.2): Bug fixes

Version numbers are **automatically determined** from
commit messages:

- `feat:` → Minor bump
- `fix:` → Patch bump
- `feat!:` or `BREAKING CHANGE:` → Major bump

### Release Process

Releases are managed by the Release Please GitHub Action:

1. **Continuous Development**: Features and fixes are
   merged to `master`
2. **Release PR Created**: Bot creates/updates PR with
   changelog and version
3. **Review**: Maintainers review the release notes
4. **Merge Release PR**: Creates git tag and GitHub release
5. **Builds**: CI automatically builds binaries for all
   platforms

### Pre-1.0.0 Versioning

PythonSCAD is currently in pre-1.0.0 stage:

- Breaking changes bump the **minor** version
  (0.6.0 → 0.7.0)
- Features bump the **patch** version (0.6.0 → 0.6.1)

This will switch to standard SemVer after 1.0.0 is released.

## Code Style

### C++ Code

- Use `clang-format` for formatting (automatically applied
  by pre-commit hooks)
- Follow existing code conventions
- Use descriptive variable names

### Python Code

- Follow PEP 8 style guide
- Use type hints where appropriate

### General Guidelines

- Keep commits focused and atomic
- Write clear, descriptive commit messages
- Add tests for new features
- Update documentation as needed
- Don't commit generated files or build artifacts

## Questions?

- **Google Group**:
  <https://groups.google.com/g/pythonscad>
- **Reddit**:
  <https://www.reddit.com/r/OpenPythonSCAD/>
- **GitHub Issues**:
  <https://github.com/pythonscad/pythonscad/issues>

---

Thank you for contributing to PythonSCAD!
