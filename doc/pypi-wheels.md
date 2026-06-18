# PyPI binary wheels

PythonSCAD publishes pre-built wheels to PyPI alongside the source
distribution. Wheels bundle the compiled `_openscad` extension and ship the
pure-Python `openscad` / `pythonscad` overlay packages (see
[`doc/python-modules.md`](python-modules.md)).

## Supported wheel matrix

| Platform tag | GitHub Actions runner | Python ABIs |
| ------------ | --------------------- | ------------- |
| `manylinux_2_28_x86_64` | `ubuntu-24.04` | cp310–cp314 |
| `manylinux_2_28_aarch64` | `ubuntu-24.04-arm` | cp310–cp314 |
| `macosx_11_0_x86_64` | `macos-15-intel` | cp310–cp314 |
| `macosx_11_0_arm64` | `macos-15` | cp310–cp314 |
| `win_amd64` | `windows-2022` | cp310–cp314 |

One manylinux wheel covers all glibc-based Linux distributions (Debian,
Ubuntu, Fedora, RHEL/Rocky, etc.) that meet the `manylinux_2_28` baseline.

## CI workflows

- **Pull requests:** [`.github/workflows/build-pypi-wheels.yml`](../.github/workflows/build-pypi-wheels.yml)
  builds a single representative wheel (`cp312-manylinux_x86_64`) and runs
  [`scripts/smoke-test-pip.py`](../scripts/smoke-test-pip.py).
- **Releases:** [`.github/workflows/publish-to-pypi.yml`](../.github/workflows/publish-to-pypi.yml)
  builds the full matrix plus sdist, then publishes everything via PyPI
  trusted publishing.

## Local development

Install pinned build tooling:

```shell
pip install -r requirements/build-pypi.txt
```

Build one wheel locally (example: Python 3.12 on Linux x86_64):

```shell
CIBW_BUILD=cp312-manylinux_x86_64 cibuildwheel --platform linux
```

Install native build dependencies first:

- **Linux:** `sudo ./scripts/get-dependencies.py --yes --profile pythonscad-pip`
- **macOS:** same command (uses Homebrew)
- **Windows:** MSVC + vcpkg via
  [`scripts/cibuildwheel/install-deps-windows.ps1`](../scripts/cibuildwheel/install-deps-windows.ps1)
  (writes `scripts/cibuildwheel/wheel-build-env.env` for `setup.py` and
  `repair-wheel-windows.ps1`; vcpkg is installed under `.wheel-vcpkg/` in the
  project root)

## Pinning and dependency updates

Wheel build tooling is pinned in [`requirements/build-pypi.txt`](../requirements/build-pypi.txt)
and bumped by Dependabot. Windows native libraries are pinned via
[`scripts/cibuildwheel/vcpkg.json`](../scripts/cibuildwheel/vcpkg.json)
(`builtin-baseline`). The `pypa/cibuildwheel` action is hash-pinned in
workflows; Dependabot's monthly GitHub Actions group covers tag bumps.
