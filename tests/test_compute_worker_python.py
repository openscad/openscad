#!/usr/bin/env python3

import subprocess
import sys
import tempfile
from pathlib import Path


with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / "model.py"
    result = Path(directory) / "result.off"
    source.write_text("from openscad import cube, show\nshow(cube([t * 14, 1, 1]))\n")
    worker = subprocess.Popen(
        [sys.argv[1], "--compute-worker"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    try:
        assert worker.stdout.readline().strip() == "ready"
        worker.stdin.write(
            f"render\t{source}\t{result}\t\tworker\t0\t0.5"
            "\t0\t0\t0\t0\t0\t0\t100\t22.5\tpython\t\n"
        )
        worker.stdin.flush()
        response = ""
        while response != "done":
            response = worker.stdout.readline().strip()
        vertices = result.read_text().splitlines()[2:10]
        assert max(float(line.split()[0]) for line in vertices) == 7
    finally:
        if worker.poll() is None:
            worker.kill()
