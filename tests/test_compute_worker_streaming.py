#!/usr/bin/env python3

"""A preview's payloads reach the GUI while the export is still running, not all at the end.

Feature 34. The worker already frames one message per distinct leaf PolySet, so the data is
incremental on the wire; what it was not is incremental in *time* -- every payload was flushed
after the export finished, immediately before the terminating control line. That forces the GUI
phase to wait for the whole worker phase, which for a model where both phases are expensive is
pure serialisation.

The assertion is about ordering, not speed: at least one payload must appear before the worker's
last progress report. Timing assertions would be flaky; ordering is exact.
"""

import json
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ipc_worker_channel import read_message  # noqa: E402

# Enough distinct leaves that the export spans several progress reports, and cheap enough to stay
# a test rather than a benchmark. Each sphere is its own leaf PolySet.
MODEL = "\n".join(
    f"translate([{i * 3}, 0, 0]) sphere(1, $fn = 48);" for i in range(40)
) + "\n"


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
            request = json.dumps({
                "command": "preview",
                "input": str(source),
                "output": str(Path(directory) / "preview.csg"),
                "normalizationLimit": 2000,
            }) + "\n"
            worker.stdin.write(request.encode())
            worker.stdin.flush()

            # Record the order messages arrive in, not their contents.
            order = []
            while True:
                message = read_message(worker)
                if message[0] == "payload":
                    order.append("payload")
                    continue
                if message[1].startswith("progress\t"):
                    order.append("progress")
                    continue
                if message[1] in ("previewdone", "done", "error", "cancelled"):
                    assert message[1] == "previewdone", f"worker replied {message[1]}"
                    break

            assert "payload" in order, "the preview sent no payloads at all"
            assert "progress" in order, "the worker reported no progress; the model is too cheap"
            first_payload = order.index("payload")
            last_progress = len(order) - 1 - order[::-1].index("progress")
            assert first_payload < last_progress, (
                "every payload arrived after the worker's last progress report, so the GUI phase "
                "cannot start until the whole worker phase is done "
                f"(first payload at {first_payload}, last progress at {last_progress}, "
                f"{order.count('payload')} payloads, {order.count('progress')} progress reports)"
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
