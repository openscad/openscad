#!/usr/bin/env python3
"""Install IPython 9.x into a --target directory on MSYS2 UCRT64.

IPython 9.x requires psutil>=7, but PyPI has no prebuilt psutil wheel
for the mingw_x86_64_ucrt_gnu-cpython ABI and source builds fail against
MSYS2's winternl.h. MSYS2 ships a pacman-built psutil; this script
installs IPython's other dependencies via pip, copies psutil from the
system site-packages into the bundle, then installs IPython itself with
--no-deps from the downloaded wheel.
"""

from __future__ import annotations

import argparse
import pathlib
import shutil
import subprocess
import tempfile

try:
    from packaging.requirements import Requirement
    from packaging.utils import parse_wheel_filename
except ModuleNotFoundError:  # minimal/MSYS2 build Pythons may lack packaging
    from pip._vendor.packaging.requirements import Requirement
    from pip._vendor.packaging.utils import parse_wheel_filename


def _run(cmd: list[str]) -> None:
    print(f"[bundle-ipython-msys2] running: {' '.join(cmd)}", flush=True)
    subprocess.run(cmd, check=True)


def _export_bundle_requirements(project_root: pathlib.Path) -> pathlib.Path:
    """Materialize the bundle dependency group as a flat requirements file."""
    if not (project_root / "pyproject.toml").is_file():
        raise SystemExit(f"pyproject.toml not found in {project_root}")
    tmp = tempfile.NamedTemporaryFile(
        mode="w",
        suffix=".txt",
        prefix="pythonscad-bundle-reqs.",
        delete=False,
        encoding="utf-8",
    )
    tmp.close()
    req_path = pathlib.Path(tmp.name)
    try:
        _run(
            [
                "uv",
                "export",
                "--project",
                str(project_root),
                "--only-group",
                "bundle",
                "--format",
                "requirements-txt",
                "--no-header",
                "--no-annotate",
                "--no-hashes",
                "-o",
                str(req_path),
            ]
        )
    except subprocess.CalledProcessError:
        req_path.unlink(missing_ok=True)
        raise
    return req_path


def _parse_runtime_requirements(requirements_file: pathlib.Path) -> tuple[list[str], list[str]]:
    """Split a requirements file into IPython specs and all other runtime deps."""
    ipython_specs: list[str] = []
    other_specs: list[str] = []
    for raw_line in requirements_file.read_text(encoding="utf-8").splitlines():
        line = raw_line.split("#", 1)[0].strip()
        if not line:
            continue
        req = Requirement(line)
        if req.name.lower() == "ipython":
            ipython_specs.append(line)
        elif req.name.lower() == "psutil":
            # Vendored from MSYS2 pacman on this code path.
            continue
        else:
            other_specs.append(line)
    return ipython_specs, other_specs


def _download_ipython_wheel(
    python: str, ipython_specs: list[str], dest_dir: pathlib.Path
) -> pathlib.Path:
    if not ipython_specs:
        raise SystemExit("requirements file contains no ipython entry")
    _run(
        [
            python,
            "-m",
            "pip",
            "download",
            "--no-deps",
            *ipython_specs,
            "-d",
            str(dest_dir),
        ]
    )
    wheels = list(dest_dir.glob("ipython-*.whl"))
    if not wheels:
        raise SystemExit(f"no ipython wheel downloaded to {dest_dir}")
    return max(wheels, key=lambda path: parse_wheel_filename(path.name)[1])


def _install_deps(python: str, target: pathlib.Path, req_strings: list[str]) -> None:
    if not req_strings:
        return
    _run(
        [
            python,
            "-m",
            "pip",
            "install",
            "--target",
            str(target),
            "--upgrade",
            "--no-compile",
            *req_strings,
        ]
    )


def _vend_psutil_from_system(target: pathlib.Path) -> None:
    import importlib.metadata

    import psutil

    src_pkg = pathlib.Path(psutil.__file__).parent
    dest_pkg = target / src_pkg.name
    if dest_pkg.exists():
        shutil.rmtree(dest_pkg)
    shutil.copytree(src_pkg, dest_pkg)

    dist = importlib.metadata.distribution("psutil")
    dist_info = pathlib.Path(getattr(dist, "_path", ""))
    if not dist_info.is_dir():
        dist_info = src_pkg.parent / f"psutil-{dist.version}.dist-info"
    if not dist_info.is_dir():
        raise SystemExit(f"psutil dist-info not found for psutil {dist.version}")

    dest_meta = target / dist_info.name
    if dest_meta.exists():
        shutil.rmtree(dest_meta)
    shutil.copytree(dist_info, dest_meta)


def _install_ipython_wheel(python: str, target: pathlib.Path, wheel_path: pathlib.Path) -> None:
    _run(
        [
            python,
            "-m",
            "pip",
            "install",
            "--target",
            str(target),
            "--no-deps",
            "--no-compile",
            str(wheel_path),
        ]
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--python", required=True, help="Python interpreter to drive pip")
    parser.add_argument("--target", required=True, type=pathlib.Path, help="Bundle staging dir")
    parser.add_argument(
        "--project",
        required=True,
        type=pathlib.Path,
        help="Project root containing pyproject.toml (bundle dependency group)",
    )
    args = parser.parse_args()

    if shutil.which("uv") is None:
        raise SystemExit(
            "uv is not on PATH. On MSYS2 UCRT64, run: pacboy -S --noconfirm uv:p"
        )

    try:
        subprocess.run([args.python, "-c", "import psutil"], check=True, capture_output=True)
    except subprocess.CalledProcessError:
        raise SystemExit(
            "psutil is not importable in the build Python. "
            "On MSYS2 UCRT64, run: pacboy -S --noconfirm python-psutil:p"
        ) from None

    args.target.mkdir(parents=True, exist_ok=True)

    requirements_file = _export_bundle_requirements(args.project.resolve())
    try:
        ipython_specs, other_specs = _parse_runtime_requirements(requirements_file)
        _install_deps(args.python, args.target, other_specs)

        with tempfile.TemporaryDirectory(prefix="ipython-wheel.") as tmp:
            wheel_dir = pathlib.Path(tmp)
            wheel = _download_ipython_wheel(args.python, ipython_specs, wheel_dir)
            _vend_psutil_from_system(args.target)
            _install_ipython_wheel(args.python, args.target, wheel)
    finally:
        requirements_file.unlink(missing_ok=True)


if __name__ == "__main__":
    main()
