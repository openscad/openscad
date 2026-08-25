#!/usr/bin/env python3

"""Reader for the compute worker's response stream (src/io/ipc_channel.cc, feature 32).

The worker answers on stdout with control lines, exactly as it always has, except that a
payload it would previously have written to a file is announced by

    payload\t<byte count>\n

followed by that many raw bytes: a framed message of [uint64 name size][name]
[uint64 payload size][payload]. Payloads contain newlines and NULs, so anything reading this
stream must read it as binary and must not use readline() across a payload.
"""

import struct

SIZE = struct.Struct("<Q")


def _read_exactly(worker, count):
    data = b""
    while len(data) < count:
        chunk = worker.stdout.read(count - len(data))
        if not chunk:
            raise RuntimeError("compute worker exited mid-payload")
        data += chunk
    return data


def read_message(worker):
    """Read one response. Returns ("line", text) or ("payload", name, bytes)."""
    line = worker.stdout.readline()
    if not line:
        raise RuntimeError("compute worker exited before replying")
    text = line.decode("utf-8", "replace").strip()
    if not text.startswith("payload\t"):
        return ("line", text)

    framed = _read_exactly(worker, int(text.split("\t", 1)[1]))
    (name_size,) = SIZE.unpack_from(framed, 0)
    offset = SIZE.size
    name = framed[offset:offset + name_size].decode("utf-8")
    offset += name_size
    (payload_size,) = SIZE.unpack_from(framed, offset)
    offset += SIZE.size
    payload = framed[offset:offset + payload_size]
    assert len(payload) == payload_size, "framed message is shorter than its length prefix"
    assert offset + payload_size == len(framed), "trailing bytes after framed message"
    return ("payload", name, payload)


def collect(worker, final):
    """Read until the given control line. Returns (lines, {name: payload bytes})."""
    lines = []
    payloads = {}
    while not lines or lines[-1] != final:
        message = read_message(worker)
        if message[0] == "line":
            lines.append(message[1])
        else:
            payloads[message[1]] = message[2]
    return lines, payloads


def payload_name(name):
    """Canonical form of a payload name, matching ipc_payload_name() in src/io/ipc_channel.cc.

    The worker names payloads after the file it would have written, but the two ends reach that
    name by different routes and spell separators differently on Windows. Both sides fold to '/'
    so a lookup cannot miss; tests must use this on the key they look up, since a Python path on
    Windows renders with backslashes.
    """
    return str(name).replace("\\", "/")
