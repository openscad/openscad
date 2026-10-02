#!/usr/bin/env python3

# Verify that PNG export honors --view transparent:
#   with the option, the file has an alpha channel, the background is fully transparent and the
#   object is fully opaque;
#   without it, the file has no alpha channel at all (some PNG consumers disagree about how to
#   interpret alpha, so we must not start emitting it unasked).
#
# This is checked by assertion rather than by comparing against an expected image, because an
# image comparison would not reliably fail if the alpha channel were silently dropped.
#
# Usage: transparent_pngtest.py <openscad-binary> <scadfile>

import os
import struct
import subprocess
import sys
import tempfile
import zlib


def read_png(path):
    """Return (width, height, channels, rows) for an 8-bit RGB or RGBA PNG."""
    with open(path, 'rb') as f:
        data = f.read()
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise AssertionError(f'{path} is not a PNG file')
    pos = 8
    idat = b''
    width = height = channels = None
    while pos + 8 <= len(data):
        length, = struct.unpack('>I', data[pos:pos + 4])
        ctype = data[pos + 4:pos + 8]
        body = data[pos + 8:pos + 8 + length]
        if ctype == b'IHDR':
            width, height, depth, color = struct.unpack('>IIBB', body[:10])
            if depth != 8:
                raise AssertionError(f'expected 8 bits per channel, got {depth}')
            if color not in (2, 6):
                raise AssertionError(f'expected RGB or RGBA, got PNG color type {color}')
            channels = 3 if color == 2 else 4
        elif ctype == b'IDAT':
            idat += body
        elif ctype == b'IEND':
            break
        pos += length + 12
    if width is None:
        raise AssertionError(f'{path} has no IHDR chunk')

    raw = zlib.decompress(idat)
    stride = width * channels
    rows = []
    prev = bytearray(stride)
    for y in range(height):
        filter_type = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - channels] if i >= channels else 0
            b = prev[i]
            c = prev[i - channels] if i >= channels else 0
            if filter_type == 0:
                pass
            elif filter_type == 1:
                line[i] = (line[i] + a) & 0xff
            elif filter_type == 2:
                line[i] = (line[i] + b) & 0xff
            elif filter_type == 3:
                line[i] = (line[i] + (a + b) // 2) & 0xff
            elif filter_type == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 0xff
            else:
                raise AssertionError(f'unknown PNG row filter {filter_type}')
        rows.append(bytes(line))
        prev = line
    return width, height, channels, rows


def render(openscad, scadfile, outfile, extra_args):
    args = [openscad, scadfile, '-o', outfile,
            '--imgsize=100,100', '--camera=80,14,19,0,0,0', '--viewall'] + extra_args
    result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode != 0:
        sys.stderr.write(result.stdout.decode('utf-8', 'replace'))
        raise AssertionError(f'OpenSCAD failed ({result.returncode}): {" ".join(args)}')
    return read_png(outfile)


def pixel(rows, channels, x, y):
    return rows[y][x * channels:(x + 1) * channels]


def main():
    if len(sys.argv) != 3:
        sys.stderr.write(f'Usage: {sys.argv[0]} <openscad-binary> <scadfile>\n')
        return 1
    openscad, scadfile = sys.argv[1], sys.argv[2]

    tempdir = tempfile.mkdtemp()
    transparent_png = os.path.join(tempdir, 'transparent.png')
    opaque_png = os.path.join(tempdir, 'opaque.png')

    width, height, channels, rows = render(openscad, scadfile, transparent_png,
                                           ['--view', 'transparent'])
    if channels != 4:
        raise AssertionError('--view transparent produced a PNG with no alpha channel')

    for corner in ((0, 0), (width - 1, 0), (0, height - 1), (width - 1, height - 1)):
        px = pixel(rows, channels, *corner)
        if px[3] != 0:
            raise AssertionError(f'background pixel at {corner} is not transparent: {tuple(px)}')

    # The model is fully opaque, so every pixel must be either background or solid object -- no
    # partial alpha anywhere. Don't assert on a specific object pixel: the test model has holes
    # through it, so which pixels show geometry depends on the camera.
    alphas = {}
    for y in range(height):
        for x in range(width):
            a = pixel(rows, channels, x, y)[3]
            alphas[a] = alphas.get(a, 0) + 1
    unexpected = sorted(a for a in alphas if a not in (0, 255))
    if unexpected:
        raise AssertionError(f'expected only alpha 0 or 255, also saw {unexpected}')
    opaque = alphas.get(255, 0)
    if opaque < (width * height) // 10:
        raise AssertionError(f'expected the object to cover the frame, only {opaque} opaque pixels')

    # The default must be unchanged: no alpha channel emitted at all.
    _, _, default_channels, _ = render(openscad, scadfile, opaque_png, [])
    if default_channels != 3:
        raise AssertionError(
            f'default PNG export gained an alpha channel ({default_channels} channels)')

    print('transparent-png: OK')
    return 0


if __name__ == '__main__':
    sys.exit(main())
