"""Identity / drop-in property checks for the openscad/pythonscad overlay.

Verifies the contract documented in ``doc/python-modules.md``:

* ``_openscad`` is the C extension; ``openscad`` re-exports it; ``pythonscad``
  re-exports ``openscad``.
* For every public name in ``openscad``, ``pythonscad.<name>`` is the same
  object (drop-in property).
* For every public name in ``_openscad``, ``openscad.<name>`` is the same
  object (no accidental shadowing today).
* ``set(dir(pythonscad)) >= set(dir(openscad)) >= set(dir(_openscad))``
  (superset relation).

If any assertion fails, this script raises ``AssertionError`` and the test
fails. On success it prints a single ``OK`` line which the regression test
captures via the echo expected file.
"""
import _openscad
import openscad
import pythonscad


def _public_names(mod):
    return {n for n in dir(mod) if not n.startswith("_")}


cext_public = _public_names(_openscad)
overlay_public = _public_names(openscad)
super_public = _public_names(pythonscad)

assert overlay_public >= cext_public, (
    "openscad must re-export every public name from _openscad; "
    "missing: " + ", ".join(sorted(cext_public - overlay_public))
)
assert super_public >= overlay_public, (
    "pythonscad must re-export every public name from openscad; "
    "missing: " + ", ".join(sorted(overlay_public - super_public))
)

for n in cext_public:
    assert getattr(openscad, n) is getattr(_openscad, n), (
        "openscad." + n + " is not the same object as _openscad." + n
    )
for n in overlay_public:
    assert getattr(pythonscad, n) is getattr(openscad, n), (
        "pythonscad." + n + " is not the same object as openscad." + n
    )

print("OK")
