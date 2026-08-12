#!/usr/bin/env python3
"""End-to-end check that --animate-threads produces the same files, in the same
places, as a sequential --animate run.

The run happens from a working directory that is NOT the .scad file's directory,
with a relative -o path. That is the case that catches a worker thread calling
chdir(): the working directory is process-global, so a thread changing it moves
every other thread's output too.

No wall-clock assertion here. Whether the parallel path is actually faster
depends on the machine and on how much of the frame is cacheable; a timing ratio
in CI is a flaky test, not a correctness test.

Usage: test_parallel_animation.py <openscad-binary> <scad-file> <output-dir>
"""

import filecmp
import os
import shutil
import subprocess
import sys

FRAMES = 4
THREADS = 4


def fail(message):
    print("FAIL: " + message, file=sys.stderr)
    sys.exit(1)


def run_animation(openscad, scad_file, cwd, output_name, threads):
    cmd = [openscad, "-o", output_name, "--animate", str(FRAMES)]
    if threads is not None:
        cmd += ["--animate-threads", str(threads)]
    cmd.append(scad_file)
    result = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    if result.returncode != 0:
        fail("%s exited %d\nstdout:\n%s\nstderr:\n%s"
             % (" ".join(cmd), result.returncode, result.stdout, result.stderr))


def frame_names(output_name):
    stem, ext = os.path.splitext(output_name)
    return ["%s%05d%s" % (stem, i, ext) for i in range(FRAMES)]


def main():
    if len(sys.argv) != 4:
        fail("usage: %s <openscad-binary> <scad-file> <output-dir>" % sys.argv[0])
    # Absolute, because every run below happens from a different working directory.
    openscad = os.path.abspath(sys.argv[1])
    scad_file = os.path.abspath(sys.argv[2])
    output_dir = os.path.abspath(sys.argv[3])

    # A working directory deliberately unrelated to the .scad file's directory.
    # Relative -o paths must resolve against this, exactly as they do without
    # --animate-threads.
    if os.path.isdir(output_dir):
        shutil.rmtree(output_dir)
    os.makedirs(output_dir)

    run_animation(openscad, scad_file, output_dir, "seq.stl", threads=None)
    run_animation(openscad, scad_file, output_dir, "par.stl", threads=THREADS)

    scad_dir = os.path.dirname(scad_file)
    for name in frame_names("seq.stl") + frame_names("par.stl"):
        produced = os.path.join(output_dir, name)
        if not os.path.exists(produced):
            stray = os.path.join(scad_dir, name)
            if os.path.exists(stray):
                os.remove(stray)
                fail("%s was written next to the .scad file (%s) instead of the "
                     "working directory - export ran with the process working "
                     "directory changed to the document directory" % (name, scad_dir))
            fail("%s was not produced" % produced)

    for seq_name, par_name in zip(frame_names("seq.stl"), frame_names("par.stl")):
        seq_path = os.path.join(output_dir, seq_name)
        par_path = os.path.join(output_dir, par_name)
        if not filecmp.cmp(seq_path, par_path, shallow=False):
            fail("%s differs from %s - parallel rendering changed the geometry"
                 % (par_name, seq_name))

    # Frames must actually differ from each other, or the comparison above would
    # pass on a run that ignored $t entirely.
    first = os.path.join(output_dir, frame_names("par.stl")[0])
    last = os.path.join(output_dir, frame_names("par.stl")[-1])
    if filecmp.cmp(first, last, shallow=False):
        fail("all frames are identical - $t was not applied per frame")

    print("PASS: %d frames, sequential and --animate-threads %d agree" % (FRAMES, THREADS))


if __name__ == "__main__":
    main()
