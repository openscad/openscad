#!/usr/bin/env python3
"""macOS Homebrew / get-dependencies installs must be Qt6-only."""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def _formula_loop(script: str) -> list[str]:
    match = re.search(
        r"for formula in ([^;]+); do",
        script,
        re.MULTILINE,
    )
    if not match:
        raise AssertionError("could not find Homebrew formula loop")
    return match.group(1).split()


def _list_packages(root: Path, *profiles: str, distro: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [
            sys.executable,
            str(root / "scripts" / "get-dependencies.py"),
            "--distro",
            distro,
            "--list",
            *[arg for profile in profiles for arg in ("--profile", profile)],
        ],
        capture_output=True,
        text=True,
        cwd=str(root),
    )


def main() -> int:
    root = _repo_root()
    script_path = root / "scripts" / "macosx-build-homebrew.sh"
    script = script_path.read_text(encoding="utf-8")

    assert "brew install qt5" not in script, "Homebrew script still installs qt5"
    assert "qt@5" not in script, "Homebrew script still references qt@5"
    assert "$USE_QT6" not in script, "Homebrew script still branches on USE_QT6"

    formulas = _formula_loop(script)
    assert "qt" in formulas, formulas
    assert "qscintilla2" in formulas, formulas
    assert "qt5" not in formulas, formulas

    rejected = _list_packages(root, "pythonscad-qt5", distro="macos")
    if rejected.returncode == 0:
        raise AssertionError(
            "pythonscad-qt5 --distro macos --list should fail\n"
            f"stdout:\n{rejected.stdout}\nstderr:\n{rejected.stderr}"
        )
    err = rejected.stderr + rejected.stdout
    assert "Qt6-only" in err, err

    also_rejected = _list_packages(root, "qt5", distro="macos")
    assert also_rejected.returncode != 0, also_rejected.stdout
    assert "Qt6-only" in (also_rejected.stderr + also_rejected.stdout)

    allowed = _list_packages(root, "pythonscad-qt6", distro="macos")
    if allowed.returncode != 0:
        raise AssertionError(
            "pythonscad-qt6 --distro macos --list should succeed\n"
            f"stdout:\n{allowed.stdout}\nstderr:\n{allowed.stderr}"
        )

    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
