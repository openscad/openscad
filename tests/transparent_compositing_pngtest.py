#!/usr/bin/env python3

# Verify the experimental `transparent-compositing` feature exports a transparent PNG that is
# actually correct for partially transparent geometry.
#
# The property under test: compositing the transparent export over the background colour must
# reproduce the ordinary opaque render, pixel for pixel. That is the whole point of a transparent
# background -- if it does not hold, the exported image cannot be placed on any other background
# without colour errors and halos.
#
# Without the feature this fails, because the buffer is cleared to the *background colour* with
# alpha 0 and blending is straight (non-premultiplied), so semi-transparent fragments end up with
# the background matted into their RGB and an alpha that is not their coverage.
#
# Usage: transparent_compositing_pngtest.py <openscad-binary> <scadfile> [--enable-feature]
#
# Correctness of the transparent *export* is required on the default path, with no feature flag:
# exporting a wrong image is not something a user should have to opt out of. Passing
# --enable-feature additionally turns on `transparent-compositing`, which makes the live view render
# the same way, and then also checks that doing so does not change how an ordinary render looks.

import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from transparent_pngtest import read_png  # noqa: E402

CAMERA = '--camera=60,40,50,0,0,0'
IMGSIZE = '--imgsize=80,80'
# Rounding through an 8-bit premultiplied buffer costs a little precision; anything larger than
# this is a real compositing error, not rounding.
TOLERANCE = 3


def render(openscad, scadfile, outfile, extra):
    args = [openscad, scadfile, '-o', outfile, IMGSIZE, CAMERA, '--viewall'] + extra
    result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode != 0:
        sys.stderr.write(result.stdout.decode('utf-8', 'replace'))
        raise AssertionError(f'OpenSCAD failed ({result.returncode}): {" ".join(args)}')
    return read_png(outfile)


def main():
    if len(sys.argv) not in (3, 4):
        sys.stderr.write(f'Usage: {sys.argv[0]} <openscad-binary> <scadfile> [--enable-feature]\n')
        return 1
    openscad, scadfile = sys.argv[1], sys.argv[2]
    use_feature = len(sys.argv) == 4 and sys.argv[3] == '--enable-feature'
    feature = ['--enable=transparent-compositing'] if use_feature else []
    tempdir = tempfile.mkdtemp()

    w, h, ch_ref, ref = render(openscad, scadfile, os.path.join(tempdir, 'opaque.png'), feature)
    if ch_ref != 3:
        raise AssertionError(f'reference render should have no alpha channel, got {ch_ref}')

    tw, th, ch, img = render(openscad, scadfile, os.path.join(tempdir, 'transparent.png'),
                             feature + ['--view', 'transparent'])
    if (tw, th) != (w, h):
        raise AssertionError('renders differ in size')
    if ch != 4:
        raise AssertionError(f'transparent render should be RGBA, got {ch} channels')

    def ref_px(x, y):
        return ref[y][x * ch_ref:(x + 1) * ch_ref]

    def img_px(x, y):
        return img[y][x * ch:(x + 1) * ch]

    # Take the background colour from the reference render's corner rather than hardcoding a
    # colour scheme.
    bg = ref_px(0, 0)

    partial = 0
    worst = (0, None)
    for y in range(h):
        for x in range(w):
            px = img_px(x, y)
            a = px[3] / 255.0
            if 0 < px[3] < 255:
                partial += 1
            expected = ref_px(x, y)
            for i in range(3):
                composited = px[i] * a + bg[i] * (1 - a)
                delta = abs(composited - expected[i])
                if delta > worst[0]:
                    worst = (delta, (x, y, tuple(px), tuple(expected), round(composited)))

    if partial < (w * h) // 100:
        raise AssertionError(
            f'test model produced almost no partial alpha ({partial} px) -- it is not exercising '
            'the case this test exists for')

    if worst[0] > TOLERANCE:
        delta, (x, y, px, expected, composited) = worst
        raise AssertionError(
            f'compositing the transparent export over the background does not reproduce the opaque '
            f'render: at ({x},{y}) exported {px} composites to {composited} per channel, expected '
            f'{expected} (off by {delta})')

    if not use_feature:
        print(f'transparent export (default path): OK ({partial} partial-alpha pixels, max '
              f'composite error {worst[0]})')
        return 0

    # Visual parity: enabling the feature must not change what an ordinary render looks like. It is
    # the whole premise of compositing the background underneath instead of clearing to it.
    _, _, ch_plain, plain = render(openscad, scadfile, os.path.join(tempdir, 'plain.png'), [])
    if ch_plain != 3:
        raise AssertionError(f'plain render should have no alpha channel, got {ch_plain}')
    parity_worst = (0, None)
    for y in range(h):
        for x in range(w):
            on = ref_px(x, y)
            off = plain[y][x * ch_plain:(x + 1) * ch_plain]
            for i in range(3):
                delta = abs(on[i] - off[i])
                if delta > parity_worst[0]:
                    parity_worst = (delta, (x, y, tuple(off), tuple(on)))
    if parity_worst[0] > TOLERANCE:
        delta, (x, y, off, on) = parity_worst
        raise AssertionError(
            f'enabling transparent-compositing changed the rendered image: at ({x},{y}) '
            f'{off} became {on} (off by {delta})')

    print(f'transparent-compositing: OK ({partial} partial-alpha pixels, max composite error '
          f'{worst[0]}, max parity error {parity_worst[0]})')
    return 0


if __name__ == '__main__':
    sys.exit(main())
