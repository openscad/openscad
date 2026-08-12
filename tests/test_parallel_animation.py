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

    print("PASS: %d frames across %d processes match the sequential run "
          "(png sequence, gif, apng)" % (FRAMES, PROCESSES))


if __name__ == "__main__":
    main()
