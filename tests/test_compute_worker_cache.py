#!/usr/bin/env python3

"""The compute worker's geometry cache survives across requests.

The worker process is persistent so that a second request for unchanged source is answered from
`GeometryCache`/`CGALCache` rather than re-evaluated. `compute_worker_export()` clears only
`StatCache` between requests, deliberately: the geometry caches are keyed by the node tree dump
and stay valid, and clearing them would throw away the per-window cache process isolation exists
to provide.

Nothing guarded that. A future change that cleared a cache too eagerly, or that made the node
tree dump differ between two identical requests, would cost every repeat render its full
evaluation time and no test would notice -- the results would still be correct, just slow.

This is a characterisation test, not a red-first one: the behaviour it pins already worked when
it was written (3.5s to 0.05s on the model below). It is here so that it keeps working.

It also prints its timings, so it doubles as the benchmark for "render, then render again with no
edits", which is the operation a user experiences as instant or not.
"""

import json
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

# Heavy enough that a cache miss is unmistakable rather than a timing wobble, cheap enough not to
# slow the suite down: ~3.5s uncached, ~0.05s cached on an M-series mac.
MODEL = "for (i = [0:60]) rotate([0, 0, i * 6]) translate([10, 0, 0]) sphere(3, $fn = 40);\n"

# The cached run must be dramatically faster, not merely faster. A broken cache re-evaluates and
# lands near 100% of the first run; a working one is a few percent. The threshold sits far from
# both so this does not become a flaky timing test on a loaded CI machine.
MAX_CACHED_FRACTION = 0.25


def render(worker, request):
    start = time.monotonic()
    worker.stdin.write(request)
    worker.stdin.flush()
    while True:
        response = worker.stdout.readline()
        if not response:
            raise RuntimeError("compute worker exited before replying")
        response = response.strip()
        if response in ("done", "previewdone", "error", "cancelled"):
            break
    assert response in ("done", "previewdone"), f"worker replied {response}"
    return time.monotonic() - start


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
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "model.scad"
            source.write_text(MODEL)
            request = json.dumps(
                {
                    "command": "render",
                    "input": str(source),
                    "output": str(Path(directory) / "result.off"),
                }
            ) + "\n"

            # The request is byte-identical each time, which is the point: the source has not
            # changed, so the node tree dump has not changed, so the cache key has not changed.
            cold = render(worker, request)
            warm = [render(worker, request) for _ in range(3)]
            best = min(warm)
            print(f"cold render: {cold:.3f}s")
            print(f"warm renders: {', '.join(f'{t:.3f}s' for t in warm)}")
            print(f"best warm / cold: {best / cold:.1%}")

            assert best < cold * MAX_CACHED_FRACTION, (
                f"repeat render of unchanged source took {best:.3f}s against a cold {cold:.3f}s "
                f"({best / cold:.1%}); the worker's geometry cache is not being hit"
            )

            # do_export() chdir()s into the document's directory, so a persistent worker used to
            # sit in the last directory it rendered from forever. On Windows a process's CWD is an
            # open handle and nothing can then remove that directory -- which is how this test
            # failed there while every assertion above passed. The worker is deliberately still
            # running here: that is the state in which the directory has to be removable.
            shutil.rmtree(directory)
            assert not Path(directory).exists(), (
                f"{directory} survived removal while the worker was still running; the worker is "
                f"holding it, most likely as its current directory"
            )

        worker.stdin.write("quit\n")
        worker.stdin.flush()
        assert worker.wait(timeout=10) == 0
    finally:
        if worker.poll() is None:
            worker.kill()


if __name__ == "__main__":
    main()
