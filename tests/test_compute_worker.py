#!/usr/bin/env python3

import subprocess
import sys
import tempfile
from pathlib import Path


def main():
    worker = subprocess.Popen(
        [sys.argv[1], "--compute-worker"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    try:
        assert worker.stdout.readline().strip() == "ready"
        worker.stdin.write("ping\n")
        worker.stdin.flush()
        assert worker.stdout.readline().strip() == "pong"
        assert worker.poll() is None

        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "model.scad"
            result = Path(directory) / "result.off"
            source.write_text("translate([1.2345678901234567, 0, 0]) cube(1);\n")
            worker.stdin.write(f"render\t{source}\t{result}\n")
            worker.stdin.flush()
            assert worker.stdout.readline().strip() == "done"
            assert result.read_text().startswith("OFF\n8 6 0\n")
            vertices = result.read_text().splitlines()[2:10]
            assert min(float(line.split()[0]) for line in vertices) == 1.2345678901234567

        worker.stdin.write("quit\n")
        worker.stdin.flush()
        assert worker.wait(timeout=5) == 0
    finally:
        if worker.poll() is None:
            worker.kill()


if __name__ == "__main__":
    main()
