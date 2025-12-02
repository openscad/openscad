# PythonSCAD Versioning System

This document describes how PythonSCAD manages versions, releases, and changelogs.

## Overview

PythonSCAD uses **automated semantic versioning** based on [Conventional Commits](https://www.conventionalcommits.org/). Version numbers are determined automatically from commit messages, eliminating manual version decisions.

## Semantic Versioning

Version format: `MAJOR.MINOR.PATCH` (e.g., `0.6.0`, `1.2.3`)

- **MAJOR**: Breaking changes (incompatible API changes)
- **MINOR**: New features (backward-compatible)
- **PATCH**: Bug fixes (backward-compatible)

### Pre-1.0.0 Behavior

During the pre-1.0.0 development phase (current state):

- Breaking changes bump **MINOR** version: `0.6.0` → `0.7.0`
- New features bump **PATCH** version: `0.6.0` → `0.6.1`
- Bug fixes bump **PATCH** version: `0.6.0` → `0.6.1`

### Post-1.0.0 Behavior

After the 1.0.0 release (stable):

- Breaking changes bump **MAJOR** version: `1.2.3` → `2.0.0`
- New features bump **MINOR** version: `1.2.3` → `1.3.0`
- Bug fixes bump **PATCH** version: `1.2.3` → `1.2.4`

## How Versions Are Determined

Versions are calculated automatically from commit message prefixes:

| Commit Type | Example | Pre-1.0.0 Bump | Post-1.0.0 Bump |
|-------------|---------|----------------|-----------------|
| `feat:` | `feat: add Python 3.12 support` | `0.6.0` → `0.6.1` | `1.2.0` → `1.3.0` |
| `fix:` | `fix: resolve memory leak` | `0.6.0` → `0.6.1` | `1.2.0` → `1.2.1` |
| `feat!:` | `feat!: redesign coordinate system` | `0.6.0` → `0.7.0` | `1.2.0` → `2.0.0` |
| `fix!:` | `fix!: correct broken API behavior` | `0.6.0` → `0.7.0` | `1.2.0` → `2.0.0` |
| Other types | `docs:`, `style:`, `refactor:`, etc. | No version bump | No version bump |

**Breaking Change Footer:**
```
feat: redesign rendering engine

BREAKING CHANGE: The origin point has changed from bottom-left to center.
All existing models need to be adjusted.
```

This also triggers a MAJOR (or MINOR in pre-1.0.0) version bump.

## Release Process

### Automated Workflow

1. **Developer makes changes**:
   - Create feature branch: `git checkout -b feature/my-feature`
   - Commit with conventional format: `git commit -m "feat: add feature"`
   - Push and create PR

2. **PR is merged to master**:
   - Merge happens immediately
   - Code is now in master branch

3. **Release Please bot activates**:
   - Analyzes all commits since last release
   - Calculates new version number
   - Creates or updates a **Release PR**

4. **Release PR contents**:
   - Updated `VERSION.txt`
   - Updated `CHANGELOG.md`
   - Summary of all changes
   - Proposed version number in PR title

5. **Maintainers review Release PR**:
   - Check if version bump is correct
   - Review changelog entries
   - Verify all features are ready for release

6. **Release PR is merged**:
   - Git tag is created (e.g., `v0.7.0`)
   - GitHub Release is published
   - Changelog is finalized

7. **CI builds release artifacts**:
   - Binaries built for all platforms
   - Attached to GitHub Release

### Batching Releases

Multiple features can be included in one release:

```
Day 1: Merge feat: add feature A  → Release PR updated to v0.7.0
Day 2: Merge fix: fix bug B        → Release PR updated to v0.7.1
Day 3: Merge feat: add feature C   → Release PR updated to v0.8.0
Day 4: Merge Release PR            → v0.8.0 released with A, B, and C
```

## Version Storage

### VERSION.txt

The canonical version is stored in `VERSION.txt` in the repository root:

```
0.6.0
```

This file is:
- Read by CMake during build
- Updated by Release Please when creating releases
- Tracked in version control

### Git Tags

Each release is tagged in git:

```bash
git tag -l
# Output:
# v0.1.0
# v0.2.0
# v0.6.0
```

Tags follow the format: `v{MAJOR}.{MINOR}.{PATCH}`

## Changelog

### CHANGELOG.md

Automatically generated and updated by Release Please. Format:

```markdown
# Changelog

## [0.7.0](https://github.com/pythonscad/pythonscad/compare/v0.6.0...v0.7.0) (2025-12-15)

### Features

* add GPU-accelerated rendering ([abc1234](https://github.com/pythonscad/pythonscad/commit/abc1234))
* support Python 3.12 ([def5678](https://github.com/pythonscad/pythonscad/commit/def5678))

### Bug Fixes

* resolve memory leak in CGAL renderer ([ghi9012](https://github.com/pythonscad/pythonscad/commit/ghi9012))
```

### Changelog Sections

Commits are grouped by type:

- **Features**: `feat:` commits
- **Bug Fixes**: `fix:` commits
- **Performance Improvements**: `perf:` commits
- **Documentation**: `docs:` commits (optional in changelog)

Other types (`style:`, `refactor:`, `test:`, `build:`, `ci:`, `chore:`) are hidden by default.

## Version in Code

### CMake

During build, CMake reads `VERSION.txt` and extracts version components:

```cmake
# From VERSION.txt: "0.6.0"
OPENSCAD_VERSION = "0.6.0"
OPENSCAD_MAJOR = 0
OPENSCAD_MINOR = 6
OPENSCAD_PATCH = 0
```

### C++ Code

Version is available in `src/version.h`:

```cpp
#include "version.h"

// Full version: "0.6.0"
std::string version = std::string(openscad_versionnumber);

// With git commit (if built from non-release): "0.6.0 (git a1b2c3d)"
std::string display_version = std::string(openscad_displayversionnumber);
```

### Runtime

Check version with:

```bash
./pythonscad --version
# Output: PythonSCAD 0.6.0
```

## Development Builds

### Between Releases

Commits merged to master but not yet released will still show the current version:

```
VERSION.txt contains: 0.6.0
Current commit: a1b2c3d (after v0.6.0 tag)
Display version: 0.6.0 (git a1b2c3d)
```

The git commit hash helps identify development builds.

### CI Builds

GitHub Actions can access the version for artifact naming:

```yaml
- name: Get version
  run: |
    VERSION=$(cat VERSION.txt)
    echo "VERSION=$VERSION" >> $GITHUB_ENV

- name: Create artifact
  run: |
    cp build/pythonscad pythonscad-${VERSION}-linux-x64
```

## Manual Overrides

### Emergency Releases

If you need to create a release without waiting for conventional commits:

1. Manually edit `VERSION.txt`
2. Commit with `chore(release): prepare version X.Y.Z`
3. Create git tag manually: `git tag vX.Y.Z`
4. Push tag: `git push origin vX.Y.Z`

⚠️ This should be rare! Prefer using the automated process.

### Skipping Release Please

If a commit shouldn't trigger a release, use types that don't bump versions:

```bash
git commit -m "docs: update README"        # No version bump
git commit -m "style: fix code formatting" # No version bump
git commit -m "chore: update dependencies" # No version bump
```

## FAQ

### Q: Can I choose my own version number?

**A:** No, versions are determined automatically from commit messages. This is by design to ensure consistency and remove subjective decisions.

### Q: What if the bot chooses the wrong version?

**A:** The bot follows strict rules. If the version seems wrong, check your commit message prefixes. You can amend commits before merging to master.

### Q: How do I create a major version bump?

**A:** Use `feat!:` or `fix!:`, or include `BREAKING CHANGE:` in the commit footer.

### Q: When will 1.0.0 be released?

**A:** Version 1.0.0 will be released when the project is stable enough for production use. It will require a commit with a breaking change (e.g., `feat!: release version 1.0.0`).

### Q: Can I see what version will be released before merging?

**A:** Yes! After merging to master, the Release PR will show the proposed version. You can review it before merging the Release PR.

### Q: What if I merge a feature but want to delay the release?

**A:** That's fine! Your feature will be in master, and the Release PR will accumulate changes. Merge the Release PR when ready to publish.

## References

- [Semantic Versioning](https://semver.org/)
- [Conventional Commits](https://www.conventionalcommits.org/)
- [Release Please](https://github.com/googleapis/release-please)
- [Contributing Guide](CONTRIBUTING.md)
