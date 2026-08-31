#!/usr/bin/env python3

"""Streaming a preview must not change *what* the preview is.

Feature 34 streams leaf payloads as the evaluation produces them instead of flushing them at the
end. `test_compute_worker_streaming.py` pins the ordering that buys, and nothing pinned the
content. The content is what the GUI draws, so it is worth a test of its own: the products index
names its leaves, and every name it uses has to have arrived and has to carry the same mesh it
would have carried without the flag.

Streaming also emits leaves the evaluator merely passed through -- the raw `square()` meshes
inside a `hull()`, whose composed result replaces them -- which the products index never
references. That is wasted bandwidth rather than a wrong picture, so it is asserted separately
and loosely: the check below is that nothing *referenced* changes, and the intermediates are
reported when they appear so the cost stays visible.
"""

import json
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ipc_worker_channel import read_message  # noqa: E402

MODELS = {}
# hull() over two squares, rounded by an offset pair, minus two circles: the composed leaf is much
# larger than the squares that go into it, so a run that references an intermediate instead of the
# result is unmistakable in the payload sizes.
MODELS["plain"] = """
linear_extrude(35) difference() {
  offset(3) offset(-3) hull() for (i = [0, 1]) translate([0, i * 45.2]) square(18.6, center = true);
  for (i = [0, 1]) translate([0, i * 45.2]) circle(4.3);
}
"""
# The same model under a root modifier, which is how this was first noticed: there the composed
# leaf is the entire picture, so referencing an intermediate shows a plainly wrong object.
MODELS["root-modifier"] = MODELS["plain"].replace("difference()", "!difference()", 1)


def preview(worker, directory, features, model):
    """(referenced leaf name -> payload size, count of payloads nothing references)."""
    source = Path(directory) / "model.scad"
    source.write_text(model)
    output = str(Path(directory) / "preview.csg")
    request = json.dumps({
        "command": "preview",
        "input": str(source),
        "output": output,
        "normalizationLimit": 2000,
        "features": features,
    }) + "\n"
    worker.stdin.write(request.encode())
    worker.stdin.flush()

    sizes, index = {}, None
    while True:
        message = read_message(worker)
        if message[0] == "payload":
            name = Path(message[1]).name
            sizes[name] = len(message[2])
            if name.endswith(".products.json"):
                index = json.loads(message[2].decode())
            continue
        if message[1].startswith("progress"):
            continue
        assert message[1] == "previewdone", f"worker replied {message[1]}"
        break

    assert index is not None, "the worker sent no products index"
    referenced = {}
    for kind in ("root", "highlights", "background"):
        for product in index.get(kind) or []:
            for side in ("intersections", "subtractions"):
                for leaf in product.get(side) or []:
                    name = Path(leaf["geometry"]).name
                    assert name in sizes, (
                        f"the products index references {name}, which never arrived"
                    )
                    # Keyed by the label rather than the payload name: the names are assigned in
                    # emission order, so streaming shifts them while meaning the same leaf.
                    referenced[leaf["label"]] = sizes[name]

    unreferenced = sum(1 for name in sizes
                       if name.endswith(".osig") and sizes[name] not in referenced.values())
    return referenced, unreferenced


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
                plain, _ = preview(worker, directory, [], model)
            with tempfile.TemporaryDirectory() as directory:
                streamed, extra = preview(
                    worker, directory, ["process-isolation", "streaming-preview"], model)

            assert streamed == plain, (
                f"streaming changed the preview's contents ({name}): without the flag {plain}, "
                f"with it {streamed}. Each entry is a leaf label and the size of the mesh the "
                "products index points it at; a changed size means the index is pointing at a "
                "different mesh than it would have."
            )
            if extra:
                print(f"note: {name} streamed {extra} leaves nothing references", file=sys.stderr)

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
