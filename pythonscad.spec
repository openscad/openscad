# PythonSCAD RPM spec file
# Built for Fedora 39+, RHEL 9+, and compatible distributions

%global debug_package %{nil}

# Filter out automatic requires for built-in libraries that aren't installed separately
%global __requires_exclude ^(libClipper2\\.so\\.1|libOpenSCADPy\\.so|libmanifold\\.so\\.3)\\(

Name:           pythonscad
Version:        %{getenv:VERSION}
Release:        1%{?dist}
Summary:        The Programmer's Solid 3D CAD Modeller

License:        GPL-2.0-or-later
URL:            https://pythonscad.org
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.16
BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  git
BuildRequires:  flex
BuildRequires:  bison
BuildRequires:  python3-devel
BuildRequires:  python3-setuptools
BuildRequires:  nettle-devel >= 3.4
BuildRequires:  nettle
BuildRequires:  boost-devel >= 1.35
BuildRequires:  tbb-devel
BuildRequires:  eigen3-devel >= 3.0
BuildRequires:  CGAL-devel >= 5.0
BuildRequires:  gmp-devel >= 5.0.0
BuildRequires:  mpfr-devel >= 3.0.0
BuildRequires:  glew-devel >= 1.5.4
BuildRequires:  opencsg-devel >= 1.3.2
BuildRequires:  harfbuzz-devel >= 0.9.19
BuildRequires:  freetype-devel >= 2.4
BuildRequires:  fontconfig-devel >= 2.10
BuildRequires:  libxml2-devel >= 2.9
BuildRequires:  double-conversion-devel
BuildRequires:  lib3mf-devel
BuildRequires:  libzip-devel
BuildRequires:  libcurl-devel
BuildRequires:  cairo-devel
BuildRequires:  gettext
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtmultimedia-devel
BuildRequires:  qt6-qtsvg-devel
BuildRequires:  qt6-qt5compat-devel
BuildRequires:  qscintilla-qt6-devel
BuildRequires:  chrpath

Requires:       python3
Requires:       qt6-qtbase
Requires:       qt6-qtmultimedia
Requires:       qt6-qtsvg
Requires:       qt6-qt5compat
Requires:       opencsg >= 1.3.2
Requires:       glew >= 1.5.4
Requires:       harfbuzz >= 0.9.19
Requires:       freetype >= 2.4
Requires:       fontconfig >= 2.10

Recommends:     meshlab

%description
PythonSCAD is a software for creating solid 3D CAD objects with Python
and OpenSCAD scripting languages.

It provides both a command-line interface and a Qt6-based GUI for
creating parametric 3D models suitable for CAD applications.

PythonSCAD is a fork of OpenSCAD with enhanced Python support and
modern features.

%prep
%setup -q

%build
%cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=%{_prefix} \
    -DCMAKE_SKIP_INSTALL_RPATH=TRUE \
    -DEXPERIMENTAL=ON \
    -DENABLE_PYTHON=ON \
    -DPYTHON_VERSION=%{python3_version} \
    -DENABLE_LIBFIVE=ON \
    -DUSE_QT6=ON \
    -DENABLE_TESTS=OFF \
    -DUSE_BUILTIN_MANIFOLD=ON \
    -DUSE_BUILTIN_CLIPPER2=ON \
    -DSNAPSHOT=OFF

%cmake_build

%install
%cmake_install

# Remove RPATH from all binaries and libraries
# This ensures clean packaging without RPATH warnings
find %{buildroot} -type f \( -name "*.so*" -o -perm /111 \) -exec chrpath --delete {} \; 2>/dev/null || true

%files
%license COPYING
%doc README.md CHANGELOG.md
%{_bindir}/pythonscad
%{_bindir}/pythonscad-python
# Bundled libraries (excluded from auto-requires on line 7)
/usr/lib/libfive.so*
/usr/lib/libClipper2.so*
/usr/lib/libOpenSCADPy.so*
/usr/lib/libmanifold.so*
%{_datadir}/pythonscad/
%{_datadir}/icons/hicolor/*/apps/pythonscad.png
%{_datadir}/applications/pythonscad.desktop
%{_datadir}/mime/packages/pythonscad.xml
%{_datadir}/metainfo/org.pythonscad.PythonSCAD.appdata.xml
%{_mandir}/man1/openscad.1*

%post
/sbin/ldconfig
update-desktop-database &> /dev/null || :
update-mime-database %{_datadir}/mime &> /dev/null || :

%postun
/sbin/ldconfig
update-desktop-database &> /dev/null || :
update-mime-database %{_datadir}/mime &> /dev/null || :

%changelog
* %{getenv:CHANGELOG_DATE} PythonSCAD Developers <noreply@pythonscad.org> - %{version}-1
- New upstream release %{version}
- See CHANGELOG.md for full details
