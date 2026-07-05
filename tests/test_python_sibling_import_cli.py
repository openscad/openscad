#!/usr/bin/env python3
"""Regression test for script-local Python imports in CLI export mode."""
from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path


TIMEOUT_SECONDS = 60
MARKER_FILE = "sibling-import-marker.txt"
HELPER_MODULE = "pythonscad_sibling_import_unique_helper"
SENTINEL = "pythonscad-local-sibling-helper"


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: test_python_sibling_import_cli.py <pythonscad-binary>", file=sys.stderr)
        return 2

    pythonscad = sys.argv[1]
    if not os.path.isfile(pythonscad):
        print(f"binary not found: {pythonscad}", file=sys.stderr)
        return 2

    with tempfile.TemporaryDirectory(prefix="pythonscad-sibling-import-") as tmp:
        root = Path(tmp)
        design_dir = root / "design"
        run_dir = root / "run-from-here"
        design_dir.mkdir()
        run_dir.mkdir()

        (design_dir / f"{HELPER_MODULE}.py").write_text(
            f"SENTINEL = {SENTINEL!r}\n"
            "def side_length():\n"
            "    return 1\n",
            encoding="utf-8",
        )
        main_py = design_dir / "main.py"
        main_py.write_text(
            "from openscad import cube, show\n"
            "from pathlib import Path\n"
            f"import {HELPER_MODULE} as local_helper\n\n"
            "Path('" + MARKER_FILE + "').write_text(\n"
            "    f'{local_helper.SENTINEL}:side={local_helper.side_length()}\\n',\n"
            "    encoding='utf-8',\n"
            ")\n"
            "show(cube([local_helper.side_length()] * 3))\n",
            encoding="utf-8",
        )

        output_file = run_dir / "out.echo"
        marker_file = run_dir / MARKER_FILE
        proc = subprocess.run(
            [
                pythonscad,
                "--trust-python",
                "-o",
                str(output_file),
                str(main_py),
            ],
            cwd=run_dir,
            capture_output=True,
            text=True,
            timeout=TIMEOUT_SECONDS,
        )
        marker = marker_file.read_text(encoding="utf-8") if marker_file.is_file() else None

    print("===== stdout =====")
    print(proc.stdout)
    print("===== stderr =====", file=sys.stderr)
    print(proc.stderr, file=sys.stderr)

    if proc.returncode != 0:
        print(
            f"FAIL: pythonscad CLI export exited with {proc.returncode}",
            file=sys.stderr,
        )
        return 1

    if marker != f"{SENTINEL}:side=1\n":
        print(
            "FAIL: sibling helper import marker file was not written correctly",
            file=sys.stderr,
        )
        print(f"marker content: {marker!r}", file=sys.stderr)
        return 1

    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
