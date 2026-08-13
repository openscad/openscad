#!/usr/bin/env python3

import subprocess
import sys
import tempfile
import json
from pathlib import Path


def wait_for(worker, final):
    responses = []
    while not responses or responses[-1] != final:
        responses.append(worker.stdout.readline().strip())
    return responses


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
            responses = wait_for(worker, "done")
            assert any(response.startswith("progress\t") for response in responses)
            assert result.read_text().startswith("OFF\n8 6 0\n")
            vertices = result.read_text().splitlines()[2:10]
            assert min(float(line.split()[0]) for line in vertices) == 1.2345678901234567

            source.write_text("translate([$t, 0, 0]) cube(1);\n")
            worker.stdin.write(f"render\t{source}\t{result}\t\tworker\t0\t0.5\n")
            worker.stdin.flush()
            wait_for(worker, "done")
            vertices = result.read_text().splitlines()[2:10]
            assert min(float(line.split()[0]) for line in vertices) == 0.5

            source.write_text(
                "translate([$vpr[0] + $vpt[0] + $vpd / 100 + $vpf / 10, 0, 0]) cube(1);\n"
            )
            worker.stdin.write(
                f"render\t{source}\t{result}\t\tworker\t0\t0.5"
                "\t1\t2\t3\t10\t20\t30\t400\t50\n"
            )
            worker.stdin.flush()
            wait_for(worker, "done")
            vertices = result.read_text().splitlines()[2:10]
            assert min(float(line.split()[0]) for line in vertices) == 20

            parameters = Path(directory) / "parameters.json"
            parameters.write_text(
                json.dumps(
                    {
                        "parameterSets": {"worker": {"size": "7"}},
                        "fileFormatVersion": "1",
                    }
                )
            )
            source.write_text("size = 1; // [1:10]\ncube(size);\n")
            worker.stdin.write(f"render\t{source}\t{result}\t{parameters}\tworker\n")
            worker.stdin.flush()
            wait_for(worker, "done")
            vertices = result.read_text().splitlines()[2:10]
            assert max(float(line.split()[0]) for line in vertices) == 7
            metadata = json.loads(Path(f"{result}.parameters.json").read_text())
            assert metadata[0]["name"] == "size"
            assert metadata[0]["type"] == "number"
            assert metadata[0]["max"] == 10
            assert metadata[0]["initial"] == 1
            assert metadata[0]["value"] == 7

            preview = Path(directory) / "preview.csg"
            source.write_text("#translate([1, 0, 0]) cube(1);\n")
            worker.stdin.write(f"preview\t{source}\t{preview}\n")
            worker.stdin.flush()
            wait_for(worker, "previewdone")
            assert "multmatrix" in preview.read_text()
            products = json.loads(Path(f"{preview}.products.json").read_text())
            assert len(products["root"]) == 1
            assert len(products["root"][0]["intersections"]) == 1
            assert len(products["highlights"]) == 1
            geometry = Path(products["root"][0]["intersections"][0]["geometry"])
            assert geometry.exists()
            assert geometry.read_text().startswith("OFF\n")

            dependency = Path(directory) / "part.scad"
            dependency.write_text("cube(2);\n")
            source.write_text("include <part.scad>\n")
            worker.stdin.write(f"preview\t{source}\t{preview}\n")
            worker.stdin.flush()
            wait_for(worker, "previewdone")
            dependencies = json.loads(Path(f"{preview}.dependencies.json").read_text())
            assert str(dependency.resolve()) in dependencies

        worker.stdin.write("quit\n")
        worker.stdin.flush()
        assert worker.wait(timeout=5) == 0
    finally:
        if worker.poll() is None:
            worker.kill()


if __name__ == "__main__":
    main()
