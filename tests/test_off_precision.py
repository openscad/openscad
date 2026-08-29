#!/usr/bin/env python3

# Checks the `-O export-off/precision=<n>` export option.
#
# OFF export writes at the iostream default of 6 significant digits, which is
# not sufficient for an exact coordinate round-trip.
#
# The option has to be opt-in: raising precision for every OFF export would move
# every existing OFF regression expectation.

import subprocess
import sys
from pathlib import Path

# A translation that needs more than 6 significant digits to survive.
OFFSET = "1.2345678901234567"


def export(binary, scad, out, *extra):
    cmd = [binary, str(scad), "-o", str(out), *extra]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        raise SystemExit(f"export failed: {' '.join(cmd)}\n{result.stderr}")
    return out.read_text()


def min_x(off_text):
    """Smallest x over the vertex block of an ASCII OFF file."""
    lines = [line for line in off_text.splitlines() if line.strip()]
    assert lines[0].strip() == "OFF", f"not an OFF file: {lines[0]!r}"
    numverts = int(lines[1].split()[0])
    return min(float(line.split()[0]) for line in lines[2:2 + numverts])


def main():
    binary, outdir = sys.argv[1], Path(sys.argv[2])
    outdir.mkdir(parents=True, exist_ok=True)

    scad = outdir / "off-precision.scad"
    scad.write_text(f"translate([{OFFSET}, 0, 0]) cube(1);\n")

    # The corner at the origin is the translation itself, exactly.
    expected = float(OFFSET)

    got = min_x(export(binary, scad, outdir / "precise.off", "-O", "export-off/precision=17"))
    if got != expected:
        raise SystemExit(
            f"export-off/precision=17 lost precision: got {got!r}, want {expected!r}")

    # The default must not move, or every existing OFF expectation moves with it.
    default = min_x(export(binary, scad, outdir / "default.off"))
    if default == expected:
        raise SystemExit(
            "default OFF export changed: it is now full precision, which "
            "invalidates the existing OFF regression expectations")

    for invalid in ("abc", "-5", "99999"):
        text = export(binary, scad, outdir / f"invalid-{invalid}.off",
                      "-O", f"export-off/precision={invalid}")
        if min_x(text) == expected:
            raise SystemExit(
                f"invalid OFF precision {invalid!r} unexpectedly enabled full precision")

    print("OK")


if __name__ == "__main__":
    main()
