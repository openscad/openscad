#!/usr/bin/env python3

"""Decoder for the compute worker's binary geometry payload (src/io/ipc_geometry.cc).

Deliberately independent of the C++ struct: if the layout changes, tests using this should
fail rather than quietly follow along. Shared by the worker protocol and export parity tests.
"""

import struct
from collections import namedtuple
from pathlib import Path

Payload = namedtuple("Payload", "vertices polygons colors color_indices metadata")

MAGIC = 0x4749534F  # "OSIG"
VERSION = 3
LIST_HEADER = "<IIII"
HEADER = "<IiIIIIII"
KIND_POLYSET = 0
KIND_POLYGON2D = 1


def read_ipc_geometry(path):
    """Return the decoded Payload from a payload file, checking it is intact."""
    return decode_ipc_geometry(Path(path).read_bytes(), path)


def decode_ipc_geometry(data, path="<channel>"):
    """Return the decoded Payload for a single-body payload; see decode_ipc_bodies."""
    bodies = decode_ipc_bodies(data, path)
    assert len(bodies) == 1, f"{path}: expected one body, got {len(bodies)}"
    return bodies[0]


def _read_string(data, offset):
    (length,) = struct.unpack_from("<I", data, offset)
    offset += 4
    return data[offset:offset + length].decode("utf-8"), offset + length


def _read_metadata(data, offset):
    (count,) = struct.unpack_from("<I", data, offset)
    offset += 4
    metadata = {}
    for _ in range(count):
        key, offset = _read_string(data, offset)
        value, offset = _read_string(data, offset)
        metadata[key] = value
    return metadata, offset


def decode_ipc_bodies(data, path="<channel>"):
    """Return a list of Payloads, one per body.

    Payloads arrive over the response channel rather than from a file (feature 32); `path` is
    only used to say which one failed.
    """
    magic, version, body_count, _reserved = struct.unpack_from(LIST_HEADER, data, 0)
    assert magic == MAGIC, f"bad magic {magic:#x} in {path}"
    assert version == VERSION, f"unexpected payload version {version}"
    offset = struct.calcsize(LIST_HEADER)
    bodies = []
    for _ in range(body_count):
        (kind,) = struct.unpack_from("<I", data, offset)
        offset += 4
        metadata, offset = _read_metadata(data, offset)
        assert kind == KIND_POLYSET, f"{path}: 2D bodies are not decoded here (kind {kind})"
        body, offset = _decode_polyset(data, offset, metadata)
        bodies.append(body)
    # Exact, not >=: this is the end-to-end guard against a payload written through a text-mode
    # stream. On Windows that rewrites every 0x0A byte -- which ordinary doubles contain -- as
    # 0x0D 0x0A, so the file is longer than its own header describes and every field after the
    # first newline byte decodes as garbage. A same-machine round trip cannot catch it; this can,
    # because it reads the worker's real output on the platform where it breaks.
    assert offset == len(data), (
        f"{path}: header describes {offset} bytes but the file holds {len(data)}"
    )
    return bodies


def _decode_polyset(data, offset, metadata):
    (_dimension, _convexity, _flags, vertex_count, polygon_count,
     _index_count, color_count, color_index_count) = struct.unpack_from(HEADER, data, offset)
    offset += struct.calcsize(HEADER)
    vertices = []
    for _ in range(vertex_count):
        vertices.append(struct.unpack_from("<3d", data, offset))
        offset += 24
    polygons = []
    for _ in range(polygon_count):
        (count,) = struct.unpack_from("<I", data, offset)
        offset += 4
        polygons.append(list(struct.unpack_from(f"<{count}i", data, offset)))
        offset += 4 * count
    colors = []
    for _ in range(color_count):
        colors.append(struct.unpack_from("<4f", data, offset))
        offset += 16
    color_indices = list(struct.unpack_from(f"<{color_index_count}i", data, offset))
    offset += 4 * color_index_count
    return Payload(vertices, polygons, colors, color_indices, metadata), offset


def write_off(payload, path):
    """Re-emit a decoded payload as OFF, at full precision.

    Lets tests that need to feed the worker's result back through OpenSCAD (which cannot
    `import()` an internal payload) keep doing so without weakening what they assert.
    """
    lines = ["OFF", f"{len(payload.vertices)} {len(payload.polygons)} 0"]
    lines += [" ".join(repr(value) for value in vertex) for vertex in payload.vertices]
    for index, polygon in enumerate(payload.polygons):
        line = f"{len(polygon)} " + " ".join(str(vertex) for vertex in polygon)
        # Per-face colour, written the way export_off does: 8-bit channels, alpha only when
        # it is not opaque. Dropping this is what made an earlier version of this helper
        # report a parity failure that was entirely its own fault.
        if payload.color_indices and index < len(payload.color_indices):
            colour_index = payload.color_indices[index]
            if colour_index >= 0:
                r, g, b, a = (round(channel * 255) for channel in payload.colors[colour_index])
                line += f" {r} {g} {b}" + (f" {a}" if a != 255 else "")
        lines.append(line)
    Path(path).write_text("\n".join(lines) + "\n")
