"""GUI-only: nimport() must emit DeprecationWarning (and still attempt download).

Headless builds omit nimport; this file is excluded from the echo suite when
HEADLESS=ON (see tests/CMakeLists.txt).

Uses a URL whose last path segment is empty, so the destination is the user
library *directory* itself. QFile::WriteOnly then fails before any file is
created or truncated (curl_download opens the path before applying its
http/https-only protocol filter).
"""

import warnings

from openscad import nimport

# Trailing slash -> empty filename -> destination is the library directory.
URL = "ftp://example.com/"

with warnings.catch_warnings(record=True) as caught:
    warnings.simplefilter("always", DeprecationWarning)
    try:
        nimport(URL)
        reached_download_error = False
    except RuntimeError:
        reached_download_error = True

warned = any(issubclass(w.category, DeprecationWarning) for w in caught)
print(f"deprecation_warning: {warned}")
print(f"reached_download_error: {reached_download_error}")
