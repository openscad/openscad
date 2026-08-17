#!/usr/bin/env python3

# Export parity between legacy in-process rendering and process isolation.
#
# In isolated mode an F6 result reaches the GUI as a binary payload that is decoded
# back into a PolySet, so an export taken after F6 goes through a round trip that
# legacy mode does not perform. This test asserts the round trip is export-invisible.
#
# OpenSCAD cannot `import()` the internal payload -- it is not a user-facing format --
# so the payload is decoded and re-emitted as OFF at full precision to stand in for
# what the GUI viewport holds. That keeps every assertion below unchanged.
#
# STL and OFF come out byte-identical. AMF and 3MF are indexed formats and the
# round trip renumbers vertices, so those are compared as geometry: the same
# triangles, expressed as coordinate triples, in any order.

import json
import re
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ipc_geometry_payload import decode_ipc_geometry, write_off  # noqa: E402
from ipc_worker_channel import read_message  # noqa: E402

MODEL = """
$fn = 24;
difference() {
  color("red") cube(10, center = true);
  sphere(6.4);
  translate([0, 0, 3]) cylinder(h = 10, r = 2);
}
"""

AMF_VERTEX = re.compile(r"<x>([^<]*)</x>\s*<y>([^<]*)</y>\s*<z>([^<]*)</z>")
AMF_TRIANGLE = re.compile(r"<v1>(\d+)</v1>\s*<v2>(\d+)</v2>\s*<v3>(\d+)</v3>")
TMF_VERTEX = re.compile(r'<vertex x="([^"]*)" y="([^"]*)" z="([^"]*)"')
TMF_TRIANGLE = re.compile(r'<triangle v1="(\d+)" v2="(\d+)" v3="(\d+)"')


def openscad(binary, *args):
    subprocess.run([binary, *args], check=True, capture_output=True)


def wait_for(worker, final):
    """Read until `final`, returning the payloads the worker sent for the request."""
    payloads = {}
    while True:
        message = read_message(worker)
        if message[0] == "payload":
            payloads[message[1]] = message[2]
            continue
        if message[1] == final:
            return payloads
        if message[1] in ("error", "cancelled"):
            raise RuntimeError(f"compute worker replied {message[1]}")


def triangles(text, vertex_pattern, triangle_pattern):
    """Triangles as sorted coordinate triples, independent of vertex numbering."""
    vertices = vertex_pattern.findall(text)
    assert vertices, "no vertices found — exporter output changed shape"
    found = sorted(
        tuple(sorted(vertices[int(index)] for index in triangle))
        for triangle in triangle_pattern.findall(text)
    )
    assert found, "no triangles found — exporter output changed shape"
    return found


def geometry(path):
    if path.suffix == ".3mf":
        text = zipfile.ZipFile(path).read("3D/3dmodel.model").decode()
        return triangles(text, TMF_VERTEX, TMF_TRIANGLE)
    return triangles(path.read_text(), AMF_VERTEX, AMF_TRIANGLE)


def main():
    binary = sys.argv[1]
    worker = subprocess.Popen(
        [binary, "--compute-worker"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    try:
        assert worker.stdout.readline().strip() == b"ready"

        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            source = directory / "model.scad"
            source.write_text(MODEL)

            handback = directory / "result.osig"
            worker.stdin.write(
                (
                    json.dumps(
                        {
                            "command": "render",
                            "input": str(source),
                            "output": str(handback),
                            "workingDirectory": str(directory),
                        }
                    )
                    + "\n"
                ).encode()
            )
            worker.stdin.flush()
            payloads = wait_for(worker, "done")
            # The handback never reaches the filesystem now; it arrives named for the path the
            # request asked for (feature 32).
            assert not handback.exists(), handback
            payload = decode_ipc_geometry(payloads[str(handback)])
            assert payload.vertices and payload.polygons

            # What the GUI viewport holds in isolated mode, expressed as something
            # OpenSCAD can read back.
            decoded = directory / "decoded.off"
            write_off(payload, decoded)
            imported = directory / "imported.scad"
            imported.write_text(f'import("{decoded.name}");\n')

            for extension in ("stl", "off", "amf", "3mf"):
                direct = directory / f"direct.{extension}"
                through_worker = directory / f"worker.{extension}"
                openscad(binary, "-o", str(direct), str(source))
                openscad(binary, "-o", str(through_worker), str(imported))
                message = f"{extension} export differs between legacy and isolated mode"
                if extension in ("stl", "off"):
                    assert direct.read_bytes() == through_worker.read_bytes(), message
                else:
                    assert geometry(direct) == geometry(through_worker), message

            worker.stdin.write(b"quit\n")
            worker.stdin.flush()
            assert worker.wait(timeout=5) == 0
    finally:
        if worker.poll() is None:
            worker.kill()


if __name__ == "__main__":
    main()
