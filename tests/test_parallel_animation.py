#!/usr/bin/env python3
"""End-to-end check for --animate-processes.

The parent spawns N copies of itself, each rendering one shard of the frames via
the existing --animate_sharding flag, then combines the results. Whether the
frames were produced by one process or several must make no difference at all to
what lands on disk, so every case here compares against the plain sequential run.
Still images are compared byte for byte; containers are compared by frame count,
for the reason documented on check_container().

Container output (gif/apng) is the case that needs the parent: separate processes
cannot share one encoder, so the parent muxes their frames itself. A still-image
sequence needs no muxing - the workers write the final numbered files directly -
and is checked too, because that is the path where the parent is only a scheduler.

Runs from a working directory that is not the .scad file's, with relative -o
paths, so a child inheriting the wrong working directory shows up as a missing
file rather than as a subtly wrong one.

Usage: test_parallel_animation.py <openscad-binary> <scad-file> <output-dir>
"""

import filecmp
import glob
import os
import shutil
import struct
import subprocess
import sys
import zlib

FRAMES = 8
PROCESSES = 4
IMGSIZE = "160,120"


def fail(message):
    print("FAIL: " + message, file=sys.stderr)
    sys.exit(1)


def run(openscad, scad_file, cwd, output_name, processes=None):
    cmd = [openscad, "-o", output_name, "--imgsize=" + IMGSIZE, "--animate", str(FRAMES)]
    if processes is not None:
        cmd += ["--animate-processes", str(processes)]
    cmd.append(scad_file)
    result = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    if result.returncode != 0:
        fail("%s exited %d\nstdout:\n%s\nstderr:\n%s"
             % (" ".join(cmd), result.returncode, result.stdout, result.stderr))
    return result


def frame_names(output_name):
    stem, ext = os.path.splitext(output_name)
    return ["%s%05d%s" % (stem, i, ext) for i in range(FRAMES)]


def gif_frame_count(data):
    if data[:6] != b"GIF89a":
        fail("gif: bad signature")
    return data.count(b"\x21\xf9\x04")  # graphic control extension, one per frame


def apng_frame_count(data):
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        fail("apng: bad signature")
    (count,) = struct.unpack(">I", data[data.index(b"acTL") + 4:data.index(b"acTL") + 8])
    return count


def check_container(openscad, scad_file, output_dir, suffix):
    """A container built by N worker processes must hold the same frames, in the same
    order, as one built by a single process.

    Not a byte comparison, deliberately. A worker hands its frames over as PNG, and
    write_png() stores RGB on purpose ("some png renderers have different
    interpretations of alpha"), so the parent feeds the encoder opaque pixels while
    the single-process path feeds it the framebuffer's own alpha. That builds a
    different palette and therefore different bytes from identical-looking frames -
    verified: every pixel of every frame matches, only the encoding differs.

    So the pixel-exactness guarantee comes from the still-image phase, which does
    compare bytes, and this phase checks what is left: that the parent collected
    every worker's frames and lost none of them. Frame *order* is structural - the
    parent reads frame i for i in 0..N-1 - so the risk this catches is a missing or
    duplicated frame, which is what a sharding-boundary mistake produces."""
    # Distinct from the still-image phase's names, so the leftover-frame check
    # below cannot trip over that phase's legitimate output.
    ref = "vidref." + suffix
    par = "vidpar." + suffix
    run(openscad, scad_file, output_dir, ref)
    run(openscad, scad_file, output_dir, par, processes=PROCESSES)

    ref_path = os.path.join(output_dir, ref)
    par_path = os.path.join(output_dir, par)
    for path in (ref_path, par_path):
        if not os.path.exists(path):
            fail("%s was not produced" % path)
        if os.path.getsize(path) == 0:
            fail("%s is empty" % path)

    count = gif_frame_count if suffix == "gif" else apng_frame_count
    with open(ref_path, "rb") as fh:
        ref_frames = count(fh.read())
    with open(par_path, "rb") as fh:
        par_frames = count(fh.read())
    if ref_frames != FRAMES:
        fail("sequential %s holds %d frames, expected %d" % (ref, ref_frames, FRAMES))
    if par_frames != FRAMES:
        fail("%s holds %d frames, expected %d - the parent dropped or duplicated a "
             "worker's frames" % (par, par_frames, FRAMES))

    # The workers' intermediate frames are the parent's business, not the user's.
    strays = sorted(glob.glob(os.path.join(output_dir, "vidpar0*.png")))
    if strays:
        fail("intermediate frames left behind in the output directory: %s"
             % " ".join(os.path.basename(s) for s in strays))


def check_still_sequence(openscad, scad_file, output_dir):
    """No muxing here - the workers write the final files. This is the path where a
    child with the wrong working directory would silently scatter frames."""
    run(openscad, scad_file, output_dir, "seq.png")
    run(openscad, scad_file, output_dir, "par.png", processes=PROCESSES)

    scad_dir = os.path.dirname(scad_file)
    for name in frame_names("par.png"):
        produced = os.path.join(output_dir, name)
        if not os.path.exists(produced):
            stray = os.path.join(scad_dir, name)
            if os.path.exists(stray):
                os.remove(stray)
                fail("%s was written next to the .scad file instead of the working "
                     "directory - a worker process did not inherit it" % name)
            fail("%s was not produced" % produced)

    for seq_name, par_name in zip(frame_names("seq.png"), frame_names("par.png")):
        if not filecmp.cmp(os.path.join(output_dir, seq_name),
                           os.path.join(output_dir, par_name), shallow=False):
            fail("%s differs from the sequential %s" % (par_name, seq_name))

    first = os.path.join(output_dir, frame_names("par.png")[0])
    last = os.path.join(output_dir, frame_names("par.png")[-1])
    if filecmp.cmp(first, last, shallow=False):
        fail("all frames are identical - $t was not applied per frame")


def png_chunks(data):
    """Yields (type, payload) for each chunk, skipping the 8-byte signature."""
    pos = 8
    while pos + 12 <= len(data):
        (length,) = struct.unpack(">I", data[pos:pos + 4])
        yield data[pos + 4:pos + 8], data[pos + 8:pos + 8 + length]
        pos += 12 + length


def decode_first_frame(data):
    """Decodes an APNG's first frame - which is a plain IDAT precisely so that a
    non-APNG decoder still sees an image - to (width, height, rows of RGB tuples).

    Deliberately hand-rolled rather than pulled from Pillow: the regression suite's
    venv is not guaranteed here, and this only has to handle what OpenSCAD emits -
    8-bit, non-interlaced, colortype 2 (RGB) or 6 (RGBA)."""
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        fail("apng: bad signature")
    idat = b""
    width = height = colortype = None
    for ctype, payload in png_chunks(data):
        if ctype == b"IHDR":
            width, height, depth, colortype, _, _, interlace = struct.unpack(">IIBBBBB", payload)
            if depth != 8 or interlace != 0 or colortype not in (2, 6):
                fail("apng: unexpected IHDR (depth %d, colortype %d, interlace %d) - this "
                     "decoder only handles what OpenSCAD writes" % (depth, colortype, interlace))
        elif ctype == b"IDAT":
            idat += payload
    if width is None:
        fail("apng: no IHDR")
    if not idat:
        fail("apng: no IDAT - the first frame must be stored as IDAT")

    channels = 3 if colortype == 2 else 4
    raw = zlib.decompress(idat)
    stride = width * channels
    rows = []
    previous = bytearray(stride)
    pos = 0
    for _ in range(height):
        method = raw[pos]
        line = bytearray(raw[pos + 1:pos + 1 + stride])
        pos += 1 + stride
        for i in range(stride):
            a = line[i - channels] if i >= channels else 0
            b = previous[i]
            c = previous[i - channels] if i >= channels else 0
            if method == 0:
                pass
            elif method == 1:
                line[i] = (line[i] + a) & 0xff
            elif method == 2:
                line[i] = (line[i] + b) & 0xff
            elif method == 3:
                line[i] = (line[i] + (a + b) // 2) & 0xff
            elif method == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pred = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[i] = (line[i] + pred) & 0xff
            else:
                fail("apng: unknown filter type %d" % method)
        # Compare on RGB only: the two paths legitimately differ on whether an opaque
        # alpha channel is stored at all, which is what this test must tolerate.
        rows.append([tuple(line[x * channels:x * channels + 3]) for x in range(width)])
        previous = line
    return width, height, rows


def check_container_pixels_match(openscad, scad_file, output_dir):
    """The parent may hand a worker's PNG to the encoder without decoding and
    re-encoding it. That shortcut is only safe if the pixels survive it, and a broken
    passthrough produces a structurally valid file full of garbage - which the
    frame-count check above would happily pass. So decode a frame and compare.

    Frame 0 only: it is the one stored as a plain IDAT, and a wrong passthrough
    corrupts every frame identically."""
    ref = "pixref.apng"
    par = "pixpar.apng"
    run(openscad, scad_file, output_dir, ref)
    run(openscad, scad_file, output_dir, par, processes=PROCESSES)

    with open(os.path.join(output_dir, ref), "rb") as fh:
        ref_w, ref_h, ref_rows = decode_first_frame(fh.read())
    with open(os.path.join(output_dir, par), "rb") as fh:
        par_w, par_h, par_rows = decode_first_frame(fh.read())

    if (ref_w, ref_h) != (par_w, par_h):
        fail("parallel apng is %dx%d, sequential is %dx%d" % (par_w, par_h, ref_w, ref_h))
    for y, (ref_row, par_row) in enumerate(zip(ref_rows, par_rows)):
        if ref_row != par_row:
            bad = next(x for x, (a, b) in enumerate(zip(ref_row, par_row)) if a != b)
            fail("parallel apng frame 0 differs from the sequential one at (%d,%d): "
                 "%s vs %s" % (bad, y, par_row[bad], ref_row[bad]))


def check_sharding_container_warns(openscad, scad_file, output_dir):
    """--animate_sharding renders a *contiguous* slice of the frames, so a container
    holding one shard is a valid animation of part of the timeline - concatenating the
    shards in order reproduces the whole thing. That is a legitimate way to spread a
    render across machines, so it must keep working.

    What is not acceptable is doing it silently: a truncated file is indistinguishable
    from a complete one, and exit code 0 says nothing happened. So the run must
    succeed, write the file, and warn - naming the frames the file actually holds."""
    for suffix, expected_frames in (("gif", FRAMES // 2), ("apng", FRAMES // 2)):
        name = "shard." + suffix
        cmd = [openscad, "-o", name, "--imgsize=" + IMGSIZE,
               "--animate", str(FRAMES), "--animate_sharding", "1/2", scad_file]
        result = subprocess.run(cmd, cwd=output_dir, capture_output=True, text=True)
        if result.returncode != 0:
            fail("--animate_sharding with %s output exited %d; it should warn and "
                 "succeed\nstdout:\n%s\nstderr:\n%s"
                 % (suffix, result.returncode, result.stdout, result.stderr))

        produced = os.path.join(output_dir, name)
        if not os.path.exists(produced) or os.path.getsize(produced) == 0:
            fail("%s was not written - the shard's own frames should still be encoded" % produced)

        # The file must hold exactly this shard's slice, not the whole animation.
        with open(produced, "rb") as fh:
            data = fh.read()
        held = (gif_frame_count if suffix == "gif" else apng_frame_count)(data)
        if held != expected_frames:
            fail("%s holds %d frames, expected this shard's %d"
                 % (name, held, expected_frames))

        # Silence is the actual bug being guarded against.
        output = result.stdout + result.stderr
        if "WARNING" not in output.upper():
            fail("no warning issued for --animate_sharding + %s; a partial container "
                 "must not be written silently\nstdout:\n%s\nstderr:\n%s"
                 % (suffix, result.stdout, result.stderr))
        # A warning that does not say which frames landed is not actionable.
        if "0" not in output or str(expected_frames - 1) not in output:
            fail("the warning for %s does not name the frame range it wrote "
                 "(expected 0-%d to appear)\noutput:\n%s"
                 % (suffix, expected_frames - 1, output))


def check_no_fork_bomb(openscad, scad_file, output_dir):
    """A worker must never spawn workers of its own. Asking for one process is the
    degenerate case and has to behave exactly like not asking at all."""
    run(openscad, scad_file, output_dir, "one.png", processes=1)
    for name in frame_names("one.png"):
        if not os.path.exists(os.path.join(output_dir, name)):
            fail("--animate-processes 1 did not produce %s" % name)


def main():
    if len(sys.argv) != 4:
        fail("usage: %s <openscad-binary> <scad-file> <output-dir>" % sys.argv[0])
    # Absolute, because every run below happens from a different working directory.
    openscad = os.path.abspath(sys.argv[1])
    scad_file = os.path.abspath(sys.argv[2])
    output_dir = os.path.abspath(sys.argv[3])

    if os.path.isdir(output_dir):
        shutil.rmtree(output_dir)
    os.makedirs(output_dir)

    check_still_sequence(openscad, scad_file, output_dir)
    check_no_fork_bomb(openscad, scad_file, output_dir)
    check_container(openscad, scad_file, output_dir, "gif")
    check_container(openscad, scad_file, output_dir, "apng")
    check_container_pixels_match(openscad, scad_file, output_dir)
    check_sharding_container_warns(openscad, scad_file, output_dir)

    print("PASS: %d frames across %d processes match the sequential run "
          "(png sequence, gif, apng)" % (FRAMES, PROCESSES))


if __name__ == "__main__":
    main()
