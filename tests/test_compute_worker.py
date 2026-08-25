#!/usr/bin/env python3

import os
import subprocess
import sys
import tempfile
import json
import io
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ipc_geometry_payload import decode_ipc_geometry  # noqa: E402
from ipc_worker_channel import collect, payload_name, read_message  # noqa: E402

# Payloads returned by the most recent wait_for(). The worker no longer writes its results to
# files (feature 32), so what used to be read back off disk is looked up here by the path the
# worker would have written -- the same string the request asked for.
PAYLOADS = {}


def send(worker, text):
    worker.stdin.write(text.encode())
    worker.stdin.flush()


def same_path(path, candidates):
    """True when `path` is one of `candidates`, comparing as paths rather than text."""
    want = os.path.normcase(os.path.realpath(path))
    return any(want == os.path.normcase(os.path.realpath(candidate)) for candidate in candidates)


def wait_for(worker, final):
    responses, payloads = collect(worker, final)
    PAYLOADS.clear()
    PAYLOADS.update(payloads)
    return responses


def wait_for_any(worker, finals):
    while True:
        message = read_message(worker)
        if message[0] == "line" and message[1] in finals:
            return message[1]


def main():
    dead_worker = type("DeadWorker", (), {"stdout": io.BytesIO(b"")})()
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
    )
    assert exiting_worker.stdout.readline().strip() == b"ready"
    send(exiting_worker, "exit-for-test\n")
    assert exiting_worker.wait(timeout=5) == 86

    worker = subprocess.Popen(
        [sys.argv[1], "--compute-worker"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    try:
        assert worker.stdout.readline().strip() == b"ready"
        send(worker, "ping\n")
        assert worker.stdout.readline().strip() == b"pong"
        assert worker.poll() is None

        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "model.scad"
            result = Path(directory) / "result.osig"
            source.write_text("translate([1.2345678901234567, 0, 0]) cube(1);\n")
            send(
                worker,
                json.dumps(
                    {
                        "command": "render",
                        "input": str(source),
                        "output": str(result),
                        "pythonVenv": "ignored\tpath\nwith controls",
                    }
                )
                + "\n",
            )
            responses = wait_for(worker, "done")
            assert any(response.startswith("progress\t") for response in responses)
            payload = decode_ipc_geometry(PAYLOADS[payload_name(result)])
            vertices, polygons = payload.vertices, payload.polygons
            assert (len(vertices), len(polygons)) == (8, 6)
            assert min(vertex[0] for vertex in vertices) == 1.2345678901234567

            source.write_text(
                'echo("first");\n'
                'echo("second");\n'
                'include <definitely-missing.scad>\n'
                'cube(1);\n'
            )
            worker.stdin.write(
                json.dumps(
                    {
                        "command": "preview",
                        "input": str(source),
                        "output": str(Path(directory) / "diagnostics.csg"),
                        "requestId": 17,
                        "features": ["structured-diagnostics"],
                    }
                )
                + "\n"
            )
            worker.stdin.flush()
            responses = wait_for(worker, "previewdone")
            diagnostic_lines = [
                response.removeprefix("diagnostic\t")
                for response in responses
                if response.startswith("diagnostic\t")
            ]
            diagnostics = [json.loads(line) for line in diagnostic_lines]
            assert [
                record["message"] for record in diagnostics if record["group"] == "echo"
            ] == ['"first"', '"second"'], diagnostics
            assert all(record["requestId"] == 17 for record in diagnostics)
            assert [record["sequence"] for record in diagnostics] == list(range(len(diagnostics)))
            assert any(
                record["group"] == "warning"
                and "definitely-missing.scad" in record["message"]
                and record["location"]["line"] == 3
                for record in diagnostics
            )
            end = responses.index("diagnostics-end\t17")
            assert all(index < end for index, response in enumerate(responses) if response.startswith("diagnostic\t"))
            assert end < responses.index("previewdone")
            assert worker.stderr.read(0) == ""

            cancel = Path(f"{result}.cancel")
            source.write_text("sphere(1, $fn=31);\n")
            cancel.touch()
            send(worker, f"render\t{source}\t{result}\n")
            assert wait_for_any(worker, {"cancelled", "done", "error"}) == "cancelled"
            cancel.unlink()
            send(worker, "ping\n")
            assert worker.stdout.readline().strip() == b"pong"

            source.write_text("translate([$t, 0, 0]) cube(1);\n")
            send(worker, f"render\t{source}\t{result}\t\tworker\t0\t0.5\n")
            wait_for(worker, "done")
            vertices = decode_ipc_geometry(PAYLOADS[payload_name(result)]).vertices
            assert min(vertex[0] for vertex in vertices) == 0.5

            source.write_text(
                "translate([$vpr[0] + $vpt[0] + $vpd / 100 + $vpf / 10, 0, 0]) cube(1);\n"
            )
            send(
                worker,
                f"render\t{source}\t{result}\t\tworker\t0\t0.5"
                "\t1\t2\t3\t10\t20\t30\t400\t50\n",
            )
            wait_for(worker, "done")
            vertices = decode_ipc_geometry(PAYLOADS[payload_name(result)]).vertices
            assert min(vertex[0] for vertex in vertices) == 20

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
            send(worker, f"render\t{source}\t{result}\t{parameters}\tworker\n")
            wait_for(worker, "done")
            vertices = decode_ipc_geometry(PAYLOADS[payload_name(result)]).vertices
            assert max(vertex[0] for vertex in vertices) == 7
            metadata = json.loads(PAYLOADS[payload_name(f"{result}.parameters.json")].decode())
            assert metadata[0]["name"] == "size"
            assert metadata[0]["type"] == "number"
            assert metadata[0]["max"] == 10
            assert metadata[0]["initial"] == 1
            assert metadata[0]["value"] == 7

            preview = Path(directory) / "preview.csg"
            source.write_text("#translate([1, 0, 0]) cube(1);\n")
            send(worker, f"preview\t{source}\t{preview}\n")
            wait_for(worker, "previewdone")
            assert "multmatrix" in PAYLOADS[payload_name(preview)].decode()
            products = json.loads(PAYLOADS[payload_name(f"{preview}.products.json")].decode())
            assert len(products["root"]) == 1
            assert any(
                node["name"].startswith("cube") and Path(node["file"]).resolve() == source.resolve()
                for node in products["nodes"]
            )
            assert len(products["root"][0]["intersections"]) == 1
            assert len(products["highlights"]) == 1
            geometry = products["root"][0]["intersections"][0]["geometry"]
            assert payload_name(geometry) in PAYLOADS, sorted(PAYLOADS)
            assert decode_ipc_geometry(PAYLOADS[payload_name(geometry)]).vertices

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
                send(worker, json.dumps(request) + "\n")
                wait_for(worker, "previewdone")
                dependencies = json.loads(PAYLOADS[payload_name(f"{preview}.dependencies.json")].decode())
                # Compared as paths, not strings: on Windows the worker and the test can spell
                # the same file "C:/dir/part.scad" or "C:\\dir\\part.scad" depending on which
                # Python is running the suite, and both are correct.
                assert same_path(dependency, dependencies), (dependency, dependencies)

            imported = document / "part.stl"
            imported.write_text(
                "solid part\n"
                "facet normal 0 0 1\nouter loop\n"
                "vertex 0 0 0\nvertex 1 0 0\nvertex 0 1 0\n"
                "endloop\nendfacet\nendsolid part\n"
            )
            source.write_text('import("part.stl");\n')
            imported_result = Path(directory) / "import.osig"
            request["command"] = "render"
            request["input"] = str(source)
            request["output"] = str(imported_result)
            send(worker, json.dumps(request) + "\n")
            wait_for(worker, "done")
            payload = decode_ipc_geometry(PAYLOADS[payload_name(imported_result)])
            vertices, polygons = payload.vertices, payload.polygons
            assert (len(vertices), len(polygons)) == (3, 1)

            source.write_text("use <MCAD/boxes.scad>\nroundedBox([2, 2, 2], 0.2, true);\n")
            request["input"] = str(source)
            request["output"] = str(result)
            send(worker, json.dumps(request) + "\n")
            wait_for(worker, "done")
            assert len(decode_ipc_geometry(PAYLOADS[payload_name(result)]).polygons) > 10

            source.write_text("translate([1, 2, 3].zyx) cube(1);\n")
            request["features"] = ["vector-swizzle"]
            send(worker, json.dumps(request) + "\n")
            wait_for(worker, "done")
            vertices = decode_ipc_geometry(PAYLOADS[payload_name(result)]).vertices
            assert min(vertex[0] for vertex in vertices) == 3

            send(worker, "quit\n")
            assert worker.wait(timeout=5) == 0
    finally:
        if worker.poll() is None:
            worker.kill()


if __name__ == "__main__":
    main()
