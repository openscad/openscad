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


# The worker speaks bytes, not text: on branches that carry the binary geometry transport a reply
# line can be followed by "payload\t<n>\n" and exactly n raw bytes, which is not valid UTF-8 and
# kills a text-mode reader. Skipping those frames keeps this test readable on every branch.
TERMINAL = (b"done", b"previewdone", b"error", b"cancelled")


def render(worker, request):
    start = time.monotonic()
    worker.stdin.write(request)
    worker.stdin.flush()
    while True:
        response = worker.stdout.readline()
        if not response:
            raise RuntimeError("compute worker exited before replying")
        response = response.strip()
        if response.startswith(b"payload\t"):
            worker.stdout.read(int(response[len(b"payload\t"):]))
            continue
        if response in TERMINAL:
            break
    assert response in (b"done", b"previewdone"), f"worker replied {response!r}"
    return time.monotonic() - start


def render_twice(binary, extra):
    """Cold and best-warm seconds for the model, in a fresh worker, with extra request fields."""
    worker = subprocess.Popen(
        [binary, "--compute-worker"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    try:
        assert worker.stdout.readline().strip() == b"ready"
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "model.scad"
            source.write_text(MODEL)
            request = json.dumps(
                {
                    "command": "render",
                    "input": str(source),
                    "output": str(Path(directory) / "result.off"),
                    **extra,
                }
            ).encode() + b"\n"
            cold = render(worker, request)
            warm = min(render(worker, request) for _ in range(2))
        worker.stdin.write(b"quit\n")
        worker.stdin.flush()
        assert worker.wait(timeout=10) == 0
        return cold, warm
    finally:
        if worker.poll() is None:
            worker.kill()


def check_cache_size_is_honoured(binary):
    """The worker must apply the cache sizes the GUI sends it.

    GeometryCache defaults to 100MB and the worker never used to change it, so the user's
    configured polyset cache size had no effect in isolated mode -- a model whose geometry
    exceeds 100MB was evicted and fully re-evaluated on every render while the same model in
    legacy mode, with a larger configured cache, was not. Sending a 1MB cache is the small,
    fast way to assert the plumbing exists: the model below does not fit in it, so nothing can
    be reused, and a worker that ignores the field caches it happily instead.
    """
    cold, warm = render_twice(binary, {"polysetCacheSizeMB": 1, "cgalCacheSizeMB": 1})
    print(f"1MB cache: cold {cold:.3f}s, warm {warm:.3f}s ({warm / cold:.1%})")
    assert warm > cold * MAX_CACHED_FRACTION, (
        f"a 1MB geometry cache still served the repeat render in {warm:.3f}s against a cold "
        f"{cold:.3f}s; the worker is ignoring the cache sizes in the request"
    )


def main():
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
            source.write_text(MODEL)
            request = json.dumps(
                {
                    "command": "render",
                    "input": str(source),
                    "output": str(Path(directory) / "result.off"),
                }
            ).encode() + b"\n"

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

        worker.stdin.write(b"quit\n")
        worker.stdin.flush()
        assert worker.wait(timeout=10) == 0
    finally:
        if worker.poll() is None:
            worker.kill()
    check_cache_size_is_honoured(sys.argv[1])


if __name__ == "__main__":
    main()
