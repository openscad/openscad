#!/usr/bin/env bash
# Install native build dependencies inside the manylinux_2_28 build container.
# Runs as cibuildwheel [tool.cibuildwheel.linux] before-all.
set -euo pipefail

echo "=== install-deps-manylinux.sh: installing pip wheel build deps ==="

# manylinux_2_28 images are AlmaLinux/RHEL 8 based.
if command -v dnf >/dev/null 2>&1; then
    PKG_MGR=dnf
else
    PKG_MGR=yum
fi

# EPEL + CRB/powertools provide CGAL and several -devel packages on EL8.
$PKG_MGR install -y epel-release
$PKG_MGR config-manager --set-enabled powertools 2>/dev/null \
    || $PKG_MGR config-manager --set-enabled crb 2>/dev/null \
    || true

$PKG_MGR install -y \
    bison \
    boost-devel \
    cairo-devel \
    CGAL-devel \
    double-conversion-devel \
    eigen3-devel \
    flex \
    fontconfig-devel \
    freetype-devel \
    gcc \
    gcc-c++ \
    glib2-devel \
    gmp-devel \
    harfbuzz-devel \
    libxml2-devel \
    mpfr-devel \
    pkgconfig

# libfive fails to compile with GCC 14's stricter C++ diagnostics; use GCC 12.
$PKG_MGR install -y gcc-toolset-12
/opt/rh/gcc-toolset-12/root/usr/bin/g++ --version

# libfive tree.cpp uses std::optional without including <optional>; EL8 libstdc++
# does not pull it in transitively.
TREE_CPP="submodules/libfive/libfive/src/tree/tree.cpp"
if [[ -f "$TREE_CPP" ]] && ! grep -q '#include <optional>' "$TREE_CPP"; then
    sed -i '/#include <stack>/a #include <optional>' "$TREE_CPP"
    echo "Patched libfive tree.cpp: added #include <optional>"
fi

echo "=== install-deps-manylinux.sh: done ==="
