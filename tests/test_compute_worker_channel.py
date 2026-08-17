#!/usr/bin/env python3

"""The compute worker returns its payloads over the response channel, not through files.

This is the end-to-end half of feature 32: ipc_channel_test.cc pins the framing in isolation,
and this pins that the worker actually uses it. The load-bearing assertion is the negative one
-- that the output directory stays empty -- because everything else here would still pass if
the worker wrote a file *and* sent a copy over the channel.
"""

import json
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ipc_geometry_payload import decode_ipc_geometry  # noqa: E402
from ipc_worker_channel import collect  # noqa: E402


def main():
    # Binary pipes, not text=True: a payload is arbitrary bytes and text mode would both mangle
    # it and split it at the newlines it contains.
    worker = subprocess.Popen(
        [sys.argv[1], "--compute-worker"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    try:
        assert worker.stdout.readline().strip() == b"ready"

        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "model.scad"
            result = Path(directory) / "result.osig"
            source.write_text("translate([1.2345678901234567, 0, 0]) cube(1);\n")
            request = {"command": "render", "input": str(source), "output": str(result)}
            worker.stdin.write((json.dumps(request) + "\n").encode())
            worker.stdin.flush()

            lines, payloads = collect(worker, "done")
            assert any(line.startswith("progress\t") for line in lines), lines

            # The payload is named for the file the worker would have written, which is what
            # lets products.json keep referring to its leaves by path.
            assert str(result) in payloads, sorted(payloads)
            geometry = decode_ipc_geometry(payloads[str(result)])
            assert (len(geometry.vertices), len(geometry.polygons)) == (8, 6)
            # Full precision survives the channel, same as it did the file.
            assert min(vertex[0] for vertex in geometry.vertices) == 1.2345678901234567

            # The point of the feature: nothing was written. Checked over the whole directory
            # rather than just the result path, so a stray sidecar or leaf file is caught too.
            leftovers = sorted(path.name for path in Path(directory).iterdir())
            assert leftovers == ["model.scad"], leftovers

        # A preview sends several payloads -- products.json plus one per distinct leaf PolySet
        # -- and none of them may touch the disk either.
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "preview.scad"
            result = Path(directory) / "preview.csg"
            source.write_text("cube(1); translate([2, 0, 0]) sphere(1, $fn=8);\n")
            request = {"command": "preview", "input": str(source), "output": str(result)}
            worker.stdin.write((json.dumps(request) + "\n").encode())
            worker.stdin.flush()

            _, payloads = collect(worker, "previewdone")
            products = f"{result}.products.json"
            assert products in payloads, sorted(payloads)
            json.loads(payloads[products].decode())
            leaves = [name for name in payloads if name.endswith(".osig")]
            assert len(leaves) == 2, sorted(payloads)
            for leaf in leaves:
                decode_ipc_geometry(payloads[leaf])

            leftovers = sorted(path.name for path in Path(directory).iterdir())
            assert leftovers == ["preview.scad"], leftovers

        worker.stdin.write(b"quit\n")
        worker.stdin.flush()
        assert worker.wait(timeout=10) == 0
    finally:
        if worker.poll() is None:
            worker.kill()
        worker.stdout.close()
        worker.stderr.close()


if __name__ == "__main__":
    main()
