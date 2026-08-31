#!/usr/bin/env python3

"""An isolated F6 must accept a 2D top-level object, exactly as the in-process path does.

`FileFormat::IPC_GEOMETRY` -- the worker's transport for a render result -- is classified as a
3D format by `fileformat::is3D()`, so `do_export()` derives `dim = 3` and `checkAndExport()`
rejects any 2D top level with "Current top level object is not a 3D object". The worker then
answers `error` and the window keeps whatever geometry it was already showing, which reads as a
stale or wrong render rather than as a failure.

The transport itself has always handled `Polygon2d` (see `appendBody` in io/ipc_geometry.cc);
only the dimension gate was wrong.

This is how the `!` root modifier appeared to be broken: `!` on a 2D subtree makes the top level
2D, so F6 in an isolated window failed for a model whose in-process F6 is fine.
"""

import json
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ipc_worker_channel import read_message  # noqa: E402

MODELS = {
    # A 2D top level reached the ordinary way.
    "plain-2d": "difference() { square(20, center = true); circle(5); }\n",
    # And reached through a root modifier on a 2D subtree, which is how this was reported.
    "root-modifier-2d": (
        "translate([50, 0, 0]) cube(5);\n"
        "linear_extrude(10) !difference() { square(20, center = true); circle(5); }\n"
    ),
    # The 3D case must keep working; without it a fix that simply removed the gate would pass.
    "3d": "difference() { cube(10, center = true); sphere(6.4, $fn = 24); }\n",
}


def render(worker, directory, model):
    """Answer the worker gives for one isolated render, plus the payload names it sent."""
    source = Path(directory) / "model.scad"
    source.write_text(model)
    request = json.dumps({
        "command": "render",
        "input": str(source),
        "output": str(Path(directory) / "model.off"),
        "features": ["process-isolation"],
    }) + "\n"
    worker.stdin.write(request.encode())
    worker.stdin.flush()

    payloads = []
    while True:
        message = read_message(worker)
        if message[0] == "payload":
            payloads.append(Path(message[1]).name)
            continue
        if message[1].startswith("progress"):
            continue
        return message[1], payloads


def main():
    worker = subprocess.Popen(
        [sys.argv[1], "--compute-worker"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    try:
        assert worker.stdout.readline().strip() == b"ready"

        for name, model in MODELS.items():
            with tempfile.TemporaryDirectory() as directory:
                answer, payloads = render(worker, directory, model)
            assert answer == "done", (
                f"isolated render of the {name} model answered {answer!r}; the window gets no "
                "geometry and keeps whatever it was already showing"
            )
            assert any(p.endswith(".off") for p in payloads), (
                f"isolated render of the {name} model answered done but sent no geometry payload "
                f"(sent {payloads})"
            )

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
