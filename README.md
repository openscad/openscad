# OpenSCAD Multi-Platform Docker Images

[![Docker Build](https://img.shields.io/github/actions/workflow/status/gounthar/openscad/docker-build-matrix.yml?branch=multiplatform&logo=docker&label=docker%20build&style=plastic)](https://github.com/gounthar/openscad/actions/workflows/docker-build-matrix.yml)
[![Dependabot](https://img.shields.io/badge/dependabot-enabled-025E8C?logo=dependabot&style=plastic)](https://github.com/gounthar/openscad/network/updates)
[![Docker Hub](https://img.shields.io/docker/pulls/gounthar/openscad?logo=docker&style=plastic)](https://hub.docker.com/r/gounthar/openscad)

Build infrastructure for creating **multi-architecture Docker images** of [OpenSCAD](https://openscad.org/).

## Supported Architectures

- `linux/amd64` (x86_64)
- `linux/arm64` (Apple Silicon, Raspberry Pi 4+)
- `linux/riscv64` (RISC-V boards)

## Quick Start

### Pull and Run

```bash
# Pull the multi-arch image (automatically selects your architecture)
docker pull gounthar/openscad:latest

# Render a SCAD file to STL
docker run --rm -v $(pwd):/work gounthar/openscad:latest /work/model.scad -o /work/output.stl

# Or use GitHub Container Registry
docker pull ghcr.io/gounthar/openscad:latest
```

### Example Usage

```bash
# Render with options
docker run --rm -v $(pwd):/work gounthar/openscad:latest \
    /work/model.scad \
    -o /work/output.stl \
    -D 'quality=50'

# Get help
docker run --rm gounthar/openscad:latest --help
```

## Repository Structure

This repository contains **only build infrastructure** - no OpenSCAD source code.

```
.
├── upstream/           # Git submodule → openscad/openscad
├── docker/             # Dockerfiles for multi-arch builds
├── packaging/          # Debian/Fedora package specs (coming soon)
├── .github/workflows/  # CI/CD pipelines
└── .updatecli/         # Automated dependency updates
```

The actual OpenSCAD source code is pulled from the [upstream repository](https://github.com/openscad/openscad) as a git submodule.

## Building Locally

### Prerequisites

- Docker with buildx support
- QEMU for cross-architecture builds (for arm64/riscv64)

### Clone and Build

```bash
# Clone with submodule
git clone --recurse-submodules https://github.com/gounthar/openscad.git
cd openscad

# Build for your current architecture
docker buildx build -f docker/Dockerfile -t openscad:local .

# Build multi-arch and push
docker buildx build \
    --platform linux/amd64,linux/arm64,linux/riscv64 \
    -f docker/Dockerfile \
    -t gounthar/openscad:latest \
    --push .
```

## Automated Updates

This repository uses automated dependency management:

- **Dependabot**: Weekly updates for Docker base images and GitHub Actions
- **UpdateCLI**: Advanced dependency tracking for Debian base image versions
- **Auto-merge**: Minor/patch updates merged automatically after CI passes

## What is OpenSCAD?

OpenSCAD is a software for creating solid 3D CAD objects. It is free software and available for Linux/UNIX, MS Windows and macOS.

Unlike most free software for creating 3D models, OpenSCAD focuses on the CAD aspects rather than the artistic aspects of 3D modeling. It's a 3D-compiler that reads script files describing objects and renders 3D models from them.

**Learn more:**
- [OpenSCAD Website](https://openscad.org/)
- [OpenSCAD Manual](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual)
- [Upstream Repository](https://github.com/openscad/openscad)

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

### Areas of Interest

- Packaging for additional Linux distributions
- Build optimizations
- Additional architecture support

## License

OpenSCAD is licensed under the GNU General Public License version 2 (GPL-2.0).

See [COPYING](COPYING) for details.
