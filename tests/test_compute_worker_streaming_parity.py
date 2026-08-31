#!/usr/bin/env python3

"""Streaming a preview must not change *what* the preview is.

Feature 34 streams leaf payloads as the evaluation produces them instead of flushing them at the
end. `test_compute_worker_streaming.py` pins the ordering that buys; nothing pinned the content,
and the content is wrong: with the flag on, the worker sends the *intermediate* meshes it walked
through -- the raw `square()` leaves inside a `hull()` -- and never sends the composed leaf that
`hull()`/`offset()` actually produce. The GUI composites what arrived, so the viewport shows the
un-hulled, un-offset shape.

It is most visible under the `!` root modifier, because then the intermediate leaves are the whole
picture and the window shows a plainly wrong object, but the modifier is not involved in the fault.

The assertion is parity: the same model, previewed with and without the flag, must yield the same
multiset of geometry payloads. Sizes are compared rather than bytes so the check does not depend on
the serialisation being byte-stable between two runs of the same binary.
"""

import json
import subprocess
import sys
import tempfile
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ipc_worker_channel import read_message  # noqa: E402

# hull() over two squares, rounded by an offset pair, minus two circles. The composed leaf is much
# larger than the squares that go into it, so a run that streams the inputs instead of the result
# is unmistakable in the payload sizes.
MODELS = {}
MODELS["plain"] = """
linear_extrude(35) difference() {
  offset(3) offset(-3) hull() for (i = [0, 1]) translate([0, i * 45.2]) square(18.6, center = true);
  for (i = [0, 1]) translate([0, i * 45.2]) circle(4.3);
}
"""
# The same model under a root modifier, which is how this was first noticed: here the composed
# leaf is not merely accompanied by the intermediates, it is missing altogether, so the window
# shows squares where it should show one rounded plate.
MODELS["root-modifier"] = MODELS["plain"].replace("difference()", "!difference()", 1)


def leaf_sizes(worker, directory, features, model):
    """The sizes of the geometry payloads one preview sends, as a multiset."""
    source = Path(directory) / "model.scad"
    source.write_text(model)
    request = json.dumps({
        "command": "preview",
        "input": str(source),
        "output": str(Path(directory) / "preview.csg"),
        "normalizationLimit": 2000,
        "features": features,
    }) + "\n"
    worker.stdin.write(request.encode())
    worker.stdin.flush()

    sizes = Counter()
    while True:
        message = read_message(worker)
        if message[0] == "payload":
            # Metadata sidecars are not geometry and are not what this compares.
            if message[1].endswith(".osig"):
                sizes[len(message[2]) if len(message) > 2 else 0] += 1
            continue
        if message[1] in ("previewdone", "done", "error", "cancelled"):
            assert message[1] == "previewdone", f"worker replied {message[1]}"
            return sizes


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
                plain = leaf_sizes(worker, directory, [], model)
            with tempfile.TemporaryDirectory() as directory:
                streamed = leaf_sizes(worker, directory,
                                      ["process-isolation", "streaming-preview"], model)

            assert streamed == plain, (
                f"streaming changed the preview's contents ({name}): "
                f"without the flag {sorted(plain.elements())}, "
                f"with it {sorted(streamed.elements())}. Payloads only present when streaming are "
                "intermediate geometry that should never have been sent; payloads only present "
                "without it are composed leaves the GUI needs."
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
