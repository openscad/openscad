#!/usr/bin/env bash
set -euo pipefail

# Emit host Python details for CMake and workflows.
# Usage: ./scripts/detect-host-python.sh >> "$GITHUB_ENV"

python3 - <<'EOF'
import sys, sysconfig, pathlib

major, minor, micro = sys.version_info[:3]
py_version = f"{major}.{minor}"
py_full_version = f"{major}.{minor}.{micro}"

include_dir = sysconfig.get_path("include")
libdir = sysconfig.get_config_var("LIBDIR") or ""
libname = sysconfig.get_config_var("LDLIBRARY") or ""
libpath = str(pathlib.Path(libdir) / libname) if libdir and libname else ""

print(f"PYTHON_VERSION={py_version}")
print(f"PYTHON_FULL_VERSION={py_full_version}")
print(f"PYTHON_INCLUDE_DIR={include_dir}")
print(f"PYTHON_LIBRARY={libpath}")
EOF
