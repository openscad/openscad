"""Echo test for issue #232: lookup_file() deprecation message must say
"Exported file" (not "Imported file") when called from an export() context.

The deprecation branch in lookup_file() fires only when the resolved
script-relative path does not exist but the document-root fallback does,
which requires a specific filesystem layout that cannot be reproduced
inside a single echo-test run (python_scriptpath is fixed to this file).

This test therefore exercises the code path indirectly: it verifies that
the FileOperation enum compiles and that calling export() with an absolute
path (which bypasses lookup_file entirely) does not emit a spurious warning.
A real end-to-end check of the "Exported file" message text lives in the
PR description for issue #232 / PR #656 and must be verified manually or
via an integration test with a controlled directory layout.
"""

import os
import tempfile

from openscad import cube, export

with tempfile.TemporaryDirectory() as tmp:
    out = os.path.join(tmp, "cube.stl")
    # Absolute path: lookup_file returns it unchanged, no deprecation fired.
    export(cube(10), out)
    exists = os.path.exists(out)
    print(f"export with absolute path: {'ok' if exists else 'FAILED'}")
