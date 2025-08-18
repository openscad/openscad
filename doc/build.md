# Building OpenSCAD from source (Ubuntu 20.04 example)

This guide shows a clean, step-by-step process to build and run OpenSCAD from source in `openscad`.

**Notes:**
- On Ubuntu 20.04, we upgrade CMake and disable Manifold to avoid a oneTBB incompatibility.
- The unified dependency script supports multiple distros; commands below focus on Ubuntu 20.04.

## 1) Clone repository and submodules

```bash
git clone https://github.com/openscad/openscad.git
cd openscad
git submodule update --init --recursive
```

## 2) Install dependencies

Use the unified installer (auto-detects distro, installs required packages):

```bash
sudo ./scripts/uni-get-dependencies.sh
```

Optional: verify versions

```bash
./scripts/check-dependencies.sh
```

Expected on Ubuntu 20.04: CGAL may be 5.0.2 (below requested 5.4). Build still works with Manifold disabled.

## 3) Install modern CMake

Ubuntu 20.04 ships CMake 3.16; submodules may require ≥ 3.18. Install via snap:

```bash
sudo snap install cmake --classic
/snap/bin/cmake --version
```

**Tip**: If `/snap/bin` isn’t first in PATH, call CMake explicitly as `/snap/bin/cmake`.

## 4) Configure the build

Minimal, compatible configuration on Ubuntu 20.04 (Manifold OFF):

```bash
/snap/bin/cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_MANIFOLD=OFF
```

Advanced (Manifold ON):
- Prefer a newer OS (Ubuntu 22.04+) so the system oneTBB works, then:

```bash
/snap/bin/cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

- Or try built-in oneTBB (may still fail on 20.04):

```bash
/snap/bin/cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMANIFOLD_USE_BUILTIN_TBB=ON
```

## 5) Build

```bash
/snap/bin/cmake --build build -j"$(nproc)"
```

## 6) Run OpenSCAD

```bash
./build/openscad
```