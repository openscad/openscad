#!/usr/bin/env python3

import subprocess
import sys
import tempfile
import json
import io
from pathlib import Path


def wait_for(worker, final):
    responses = []
    while not responses or responses[-1] != final:
        response = worker.stdout.readline()
        if not response:
            raise RuntimeError("compute worker exited before replying")
        responses.append(response.strip())
    return responses


def wait_for_any(worker, finals):
    while True:
        response = worker.stdout.readline()
        if not response:
            raise RuntimeError("compute worker exited before replying")
        response = response.strip()
        if response in finals:
            return response


def main():
    dead_worker = type("DeadWorker", (), {"stdout": io.StringIO("")})()
    try:
        wait_for(dead_worker, "done")
        raise AssertionError("EOF should fail immediately")
    except RuntimeError:
        pass

    exiting_worker = subprocess.Popen(
        [sys.argv[1], "--compute-worker"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    assert exiting_worker.stdout.readline().strip() == "ready"
    exiting_worker.stdin.write("exit-for-test\n")
    exiting_worker.stdin.flush()
    assert exiting_worker.wait(timeout=5) == 86

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
            worker.stdin.write(
                json.dumps(
                    {
                        "command": "render",
                        "input": str(source),
                        "output": str(result),
                        "pythonVenv": "ignored\tpath\nwith controls",
                    }
                )
                + "\n"
            )
            worker.stdin.flush()
            responses = wait_for(worker, "done")
            assert any(response.startswith("progress\t") for response in responses)
            assert result.read_text().startswith("OFF\n8 6 0\n")
            vertices = result.read_text().splitlines()[2:10]
            assert min(float(line.split()[0]) for line in vertices) == 1.2345678901234567

            cancel = Path(f"{result}.cancel")
            source.write_text("sphere(1, $fn=31);\n")
            cancel.touch()
            worker.stdin.write(f"render\t{source}\t{result}\n")
            worker.stdin.flush()
            assert wait_for_any(worker, {"cancelled", "done", "error"}) == "cancelled"
            cancel.unlink()
            worker.stdin.write("ping\n")
            worker.stdin.flush()
            assert worker.stdout.readline().strip() == "pong"

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
            assert any(
                node["name"].startswith("cube") and Path(node["file"]).resolve() == source.resolve()
                for node in products["nodes"]
            )
            assert len(products["root"][0]["intersections"]) == 1
            assert len(products["highlights"]) == 1
            geometry = Path(products["root"][0]["intersections"][0]["geometry"])
            assert geometry.exists()
            assert geometry.read_text().startswith("OFF\n")

            document = Path(directory) / "document"
            document.mkdir()
            dependency = document / "part.scad"
            dependency.write_text("module part() { cube(2); }\n")
            for directive in ("include <part.scad>\npart();\n", "use <part.scad>\npart();\n"):
                source.write_text(directive)
                request = {
                    "command": "preview",
                    "input": str(source),
                    "output": str(preview),
                    "workingDirectory": str(document),
                }
                worker.stdin.write(json.dumps(request) + "\n")
                worker.stdin.flush()
                wait_for(worker, "previewdone")
                dependencies = json.loads(Path(f"{preview}.dependencies.json").read_text())
                assert str(dependency.resolve()) in dependencies

            imported = document / "part.stl"
            imported.write_text(
                "solid part\n"
                "facet normal 0 0 1\nouter loop\n"
                "vertex 0 0 0\nvertex 1 0 0\nvertex 0 1 0\n"
                "endloop\nendfacet\nendsolid part\n"
            )
            source.write_text('import("part.stl");\n')
            imported_result = Path(directory) / "import.off"
            request["command"] = "render"
            request["input"] = str(source)
            request["output"] = str(imported_result)
            worker.stdin.write(json.dumps(request) + "\n")
            worker.stdin.flush()
            wait_for(worker, "done")
            assert imported_result.read_text().startswith("OFF\n3 1 0\n")

            source.write_text("use <MCAD/boxes.scad>\nroundedBox([2, 2, 2], 0.2, true);\n")
            request["input"] = str(source)
            request["output"] = str(result)
            worker.stdin.write(json.dumps(request) + "\n")
            worker.stdin.flush()
            wait_for(worker, "done")
            assert len(result.read_text().splitlines()) > 10

            source.write_text("translate([1, 2, 3].zyx) cube(1);\n")
            request["features"] = ["vector-swizzle"]
            worker.stdin.write(json.dumps(request) + "\n")
            worker.stdin.flush()
            wait_for(worker, "done")
            vertices = result.read_text().splitlines()[2:10]
            assert min(float(line.split()[0]) for line in vertices) == 3

            worker.stdin.write("quit\n")
            worker.stdin.flush()
            assert worker.wait(timeout=5) == 0
    finally:
        if worker.poll() is None:
            worker.kill()


if __name__ == "__main__":
    main()
