#!/usr/bin/env python3

import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ipc_geometry_payload import decode_ipc_geometry  # noqa: E402
from ipc_worker_channel import collect, payload_name  # noqa: E402


with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / "model.py"
    result = Path(directory) / "result.osig"
    source.write_text("from openscad import cube, show\nshow(cube([t * 14, 1, 1]))\n")
    worker = subprocess.Popen(
        [sys.argv[1], "--compute-worker"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    try:
        assert worker.stdout.readline().strip() == b"ready"
        worker.stdin.write(
            (
                f"render\t{source}\t{result}\t\tworker\t0\t0.5"
                "\t0\t0\t0\t0\t0\t0\t100\t22.5\tpython\t\n"
            ).encode()
        )
        worker.stdin.flush()
        # The result comes back over the response channel rather than as a file (feature 32).
        _, payloads = collect(worker, "done")
        vertices = decode_ipc_geometry(payloads[payload_name(result)]).vertices
        assert max(vertex[0] for vertex in vertices) == 7
    finally:
        if worker.poll() is None:
            worker.kill()
