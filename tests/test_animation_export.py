#!/usr/bin/env python3
"""End-to-end check for the animation container formats (gif, apng, avi).

Runs the real binary with --animate and validates the container it produced by
parsing it, rather than comparing against a golden file: the compressed bytes vary
with the zlib and JPEG versions in use, but the structure and the frame count do not.

Usage: test_animation_export.py <openscad-binary> <scad-file> <output-dir>
"""

import os
import struct
import subprocess
import sys
import tempfile

FRAMES = 4
FPS = 10
WIDTH = 64
HEIGHT = 48


def fail(message):
    print("FAIL: " + message, file=sys.stderr)
    sys.exit(1)


def check_apng(data):
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        fail("apng: bad signature")

    counts = {}
    frame_count = None
    pos = 8
    while pos + 12 <= len(data):
        (length,) = struct.unpack(">I", data[pos:pos + 4])
        chunk_type = data[pos + 4:pos + 8]
        payload = data[pos + 8:pos + 8 + length]
        counts[chunk_type] = counts.get(chunk_type, 0) + 1
        if chunk_type == b"acTL":
            (frame_count,) = struct.unpack(">I", payload[:4])
        pos += 12 + length
    if pos != len(data):
        fail("apng: trailing bytes after the last chunk")
    if counts.get(b"acTL") != 1:
        fail("apng: expected exactly one acTL, got %r" % counts.get(b"acTL"))
    if frame_count != FRAMES:
        fail("apng: acTL says %r frames, expected %d" % (frame_count, FRAMES))
    if counts.get(b"fcTL") != FRAMES:
        fail("apng: expected %d fcTL, got %r" % (FRAMES, counts.get(b"fcTL")))
    if counts.get(b"fdAT") != FRAMES - 1:
        fail("apng: expected %d fdAT, got %r" % (FRAMES - 1, counts.get(b"fdAT")))


def check_gif(data):
    if data[:6] != b"GIF89a":
        fail("gif: bad signature")
    if data[-1] != 0x3B:
        fail("gif: missing trailer")
    if b"NETSCAPE2.0" not in data:
        fail("gif: missing the NETSCAPE2.0 loop extension")

    width, height = struct.unpack("<HH", data[6:10])
    if (width, height) != (WIDTH, HEIGHT):
        fail("gif: logical screen is %dx%d, expected %dx%d" % (width, height, WIDTH, HEIGHT))

    # Count image descriptors by walking the block stream.
    packed = data[10]
    pos = 13
    if packed & 0x80:
        pos += 3 << ((packed & 0x07) + 1)

    def skip_sub_blocks(pos):
        while pos < len(data):
            size = data[pos]
            pos += 1 + size
            if size == 0:
                return pos
        fail("gif: unterminated sub-block chain")

    descriptors = 0
    while pos < len(data):
        introducer = data[pos]
        if introducer == 0x3B:
            pos += 1
            break
        if introducer == 0x21:
            pos = skip_sub_blocks(pos + 2)
        elif introducer == 0x2C:
            descriptors += 1
            local = data[pos + 9]
            pos += 10
            if local & 0x80:
                pos += 3 << ((local & 0x07) + 1)
            pos = skip_sub_blocks(pos + 1)
        else:
            fail("gif: unexpected block introducer 0x%02x at %d" % (introducer, pos))
    if descriptors != FRAMES:
        fail("gif: %d image descriptors, expected %d" % (descriptors, FRAMES))


def check_avi(data):
    if data[:4] != b"RIFF" or data[8:12] != b"AVI ":
        fail("avi: not a RIFF AVI")
    (riff_size,) = struct.unpack("<I", data[4:8])
    if riff_size != len(data) - 8:
        fail("avi: RIFF size %d, expected %d" % (riff_size, len(data) - 8))

    (total_frames,) = struct.unpack("<I", data[48:52])
    if total_frames != FRAMES:
        fail("avi: avih dwTotalFrames %d, expected %d" % (total_frames, FRAMES))
    if data[108 + 4:108 + 8] != b"MJPG":
        fail("avi: stream handler is %r, expected MJPG" % data[112:116])

    movi_base = data.find(b"movi")
    if movi_base < 0:
        fail("avi: no movi list")
    idx = data.find(b"idx1", movi_base)
    if idx < 0:
        fail("avi: no idx1 index")
    (idx_size,) = struct.unpack("<I", data[idx + 4:idx + 8])
    if idx_size != FRAMES * 16:
        fail("avi: idx1 holds %d entries, expected %d" % (idx_size // 16, FRAMES))

    # Every index entry must resolve to the chunk it claims -- a bogus index still
    # opens in some players and fails in others.
    for i in range(FRAMES):
        entry = idx + 8 + i * 16
        ckid = data[entry:entry + 4]
        offset, length = struct.unpack("<II", data[entry + 8:entry + 16])
        if ckid != b"00dc":
            fail("avi: idx1 entry %d names %r" % (i, ckid))
        at = movi_base + offset
        if data[at:at + 4] != b"00dc":
            fail("avi: idx1 entry %d points at %r, not a 00dc chunk" % (i, data[at:at + 4]))
        (declared,) = struct.unpack("<I", data[at + 4:at + 8])
        if declared != length:
            fail("avi: idx1 entry %d says %d bytes, chunk says %d" % (i, length, declared))
        if data[at + 8:at + 10] != b"\xff\xd8":
            fail("avi: frame %d does not start with a JPEG SOI marker" % i)


CHECKERS = {"gif": check_gif, "apng": check_apng, "avi": check_avi}


def main():
    if len(sys.argv) != 4:
        print(__doc__, file=sys.stderr)
        return 1
    binary, scad_file, output_dir = sys.argv[1:]
    os.makedirs(output_dir, exist_ok=True)

    for suffix, checker in sorted(CHECKERS.items()):
        output = os.path.join(output_dir, "animation_export." + suffix)
        if os.path.exists(output):
            os.remove(output)
        command = [
            binary, scad_file,
            "--animate", str(FRAMES),
            "--animate_fps", str(FPS),
            "--imgsize=%d,%d" % (WIDTH, HEIGHT),
            "-o", output,
        ]
        result = subprocess.run(command, capture_output=True)
        if result.returncode != 0:
            fail("%s: exit %d\n%s" % (suffix, result.returncode,
                                      result.stderr.decode("utf-8", "replace")))
        if not os.path.exists(output):
            fail("%s: no output file was written" % suffix)

        with open(output, "rb") as handle:
            data = handle.read()
        if not data:
            fail("%s: output file is empty" % suffix)
        checker(data)
        print("ok: %s (%d bytes)" % (suffix, len(data)))

    return 0


if __name__ == "__main__":
    sys.exit(main())
