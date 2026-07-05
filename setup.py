from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import subprocess
import os
import re
import shutil
import sys


IS_WINDOWS = sys.platform == "win32"
IS_DARWIN = sys.platform == "darwin"


def apply_wheel_build_env():
    """Load env written by cibuildwheel before-all hooks (needed on Windows)."""
    here = os.path.dirname(os.path.abspath(__file__))
    env_file = os.path.join(here, "scripts", "cibuildwheel", "wheel-build-env.env")
    if not os.path.isfile(env_file):
        return
    with open(env_file, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            key, _, value = line.partition("=")
            key = key.strip()
            value = value.strip()
            if key and value:
                os.environ[key] = value
    winflex = os.environ.get("WINFLEXBISON_DIR")
    if winflex and os.path.isdir(winflex):
        os.environ["PATH"] = winflex + os.pathsep + os.environ.get("PATH", "")


def get_version():
    here = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(here, "VERSION.txt")) as f:
        return f.read().strip()


def get_version_parts():
    version = get_version()
    match = re.match(r"^(([0-9]+)\.([0-9]+)(?:\.([0-9]+))?)(?:-([^+]+))?(?:\+(.*))?$", version)
    if not match:
        raise RuntimeError(f"Version string '{version}' doesn't match expected semantic version format")

    major = str(int(match.group(2)))
    minor = str(int(match.group(3)))
    patch = str(int(match.group(4) or "0"))
    return version, match.group(1), major, minor, patch


def pkg_config_cmd():
    """Return the pkg-config executable (supports vcpkg pkgconf on Windows)."""
    return os.environ.get("PKG_CONFIG", "pkg-config")


def pkg_config_flags(packages, flag):
    """Query pkg-config for the given flag (e.g. '--cflags-only-I', '--libs-only-l')."""
    try:
        out = subprocess.check_output(
            [pkg_config_cmd(), flag] + packages,
            text=True, stderr=subprocess.DEVNULL,
            env=os.environ,
        )
        return out.strip().split()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return []


def pkg_config_exists(package):
    """Return True if pkg-config can find the given package."""
    try:
        subprocess.check_call(
            [pkg_config_cmd(), "--exists", package],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
            env=os.environ,
        )
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def pkg_config_version(package):
    """Return the version string for a pkg-config package, or None."""
    try:
        out = subprocess.check_output(
            [pkg_config_cmd(), "--modversion", package],
            text=True, stderr=subprocess.DEVNULL,
            env=os.environ,
        )
        return out.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None


def get_vcpkg_installed_dir():
    """Return the vcpkg installed prefix for the active triplet, if any."""
    root = os.environ.get("VCPKG_ROOT") or os.environ.get("PYTHONSCAD_VCPKG_ROOT")
    if not root:
        return None
    triplet = os.environ.get("VCPKG_DEFAULT_TRIPLET", "x64-windows")
    installed = os.path.join(root, "installed", triplet)
    return installed if os.path.isdir(installed) else None


def get_pkg_config_include_dirs():
    """Return include directories discovered via pkg-config."""
    pkgs = ["eigen3", "harfbuzz", "libxml-2.0", "freetype2", "glib-2.0", "cairo"]
    raw = pkg_config_flags(pkgs, "--cflags-only-I")
    dirs = [flag[2:] for flag in raw if flag.startswith("-I")]

    # Fallback paths used when pkg-config is unavailable or incomplete
    fallbacks = [
        "/usr/include/eigen3",
        "/usr/include/harfbuzz",
        "/usr/include/libxml2",
        "/usr/include/freetype2",
        "/usr/include/glib-2.0",
        "/usr/include/cairo",
        "/usr/lib64/glib-2.0/include",
        "/usr/lib/x86_64-linux-gnu/glib-2.0/include",
        "/usr/lib/aarch64-linux-gnu/glib-2.0/include",
        "/opt/homebrew/include",
        "/opt/homebrew/include/eigen3",
        "/opt/homebrew/include/harfbuzz",
        "/opt/homebrew/include/freetype2",
        "/opt/homebrew/include/glib-2.0",
        "/opt/homebrew/include/cairo",
        "/opt/homebrew/lib/glib-2.0/include",
        "/usr/local/include",
        "/usr/local/include/eigen3",
        "/usr/local/include/harfbuzz",
        "/usr/local/include/freetype2",
        "/usr/local/include/glib-2.0",
        "/usr/local/include/cairo",
        "/usr/local/lib/glib-2.0/include",
    ]
    vcpkg_installed = get_vcpkg_installed_dir()
    if vcpkg_installed:
        fallbacks.insert(0, os.path.join(vcpkg_installed, "include"))
    for fb in fallbacks:
        if fb not in dirs and os.path.isdir(fb):
            dirs.append(fb)

    return dirs


def get_pkg_config_libraries():
    """Return library names discovered via pkg-config."""
    pkgs = ["freetype2", "libxml-2.0", "fontconfig", "glib-2.0", "harfbuzz"]
    raw = pkg_config_flags(pkgs, "--libs-only-l")
    libs = [flag[2:] for flag in raw if flag.startswith("-l")]

    required = ["freetype", "xml2", "fontconfig", "double-conversion", "gmp",
                "harfbuzz", "mpfr", "glib-2.0"]
    for lib in required:
        if lib not in libs:
            libs.append(lib)

    return libs


def get_link_libraries():
    """Return the full set of libraries to link the pip extension against."""
    libs = get_pkg_config_libraries()

    # Match CMake (Boost::regex, Boost::program_options) so shared deps appear
    # in DT_NEEDED and auditwheel/delocate/delvewheel can bundle them.
    for lib in ("boost_regex", "boost_program_options", "boost_system"):
        if lib not in libs:
            libs.append(lib)

    cgal_raw = pkg_config_flags(["CGAL"], "--libs-only-l")
    if cgal_raw:
        for flag in cgal_raw:
            if flag.startswith("-l") and flag[2:] not in libs:
                libs.append(flag[2:])
    else:
        # CGAL 5+ is header-only on many distros (e.g. EL8); GMP/MPFR suffice.
        for lib in ("CGAL", "CGAL_Core"):
            for libdir in ("/usr/lib64", "/usr/lib"):
                if any(
                    os.path.isfile(os.path.join(libdir, f"lib{lib}{suffix}"))
                    for suffix in (".so", ".a")
                ):
                    if lib not in libs:
                        libs.append(lib)
                    break

    return libs


def get_library_dirs():
    """Return linker search paths for the active platform."""
    dirs = []
    vcpkg_installed = get_vcpkg_installed_dir()
    if vcpkg_installed:
        dirs.append(os.path.join(vcpkg_installed, "lib"))
    if IS_DARWIN:
        for prefix in ("/opt/homebrew", "/usr/local"):
            libdir = os.path.join(prefix, "lib")
            if os.path.isdir(libdir):
                dirs.append(libdir)
    elif not IS_WINDOWS:
        for libdir in ("/usr/lib64", "/usr/lib"):
            if os.path.isdir(libdir):
                dirs.append(libdir)
    return dirs


def get_extra_compile_args():
    """Return platform-specific compiler flags."""
    if IS_WINDOWS:
        return ["/bigobj", "/EHsc", "/std:c++17"]
    args = ["-std=c++17"]
    if IS_DARWIN:
        args.append("-stdlib=libc++")
    return args


def get_platform_sources():
    """Return platform-specific utility sources."""
    sources = ["src/platform/PlatformUtils.cc"]
    if IS_WINDOWS:
        sources.append("src/platform/PlatformUtils-win.cc")
    elif IS_DARWIN:
        sources.append("src/platform/PlatformUtils-mac.mm")
    else:
        sources.append("src/platform/PlatformUtils-posix.cc")
    return sources


def get_extra_link_args():
    """Return platform-specific linker flags."""
    if IS_DARWIN:
        return ["-framework", "Foundation"]
    return []


def get_extra_defines():
    """Return platform-specific preprocessor defines."""
    defines = []
    if IS_WINDOWS:
        defines.extend([
            ("NOMINMAX", None),
            ("_USE_MATH_DEFINES", None),
            ("CGAL_DISABLE_ROUNDING_MATH_CHECK", None),
        ])
    return defines


def detect_lib3mf():
    """Detect lib3mf and return (sources, include_dirs, libraries, defines) or dummy fallback."""
    # lib3mf v2 uses pkg-config name "lib3mf", v1 uses "lib3MF"
    for pkg_name in ("lib3mf", "lib3MF"):
        ver = pkg_config_version(pkg_name)
        if ver is None:
            continue

        inc_raw = pkg_config_flags([pkg_name], "--cflags-only-I")
        inc_dirs = [f[2:] for f in inc_raw if f.startswith("-I")]
        lib_raw = pkg_config_flags([pkg_name], "--libs-only-l")
        libraries = [f[2:] for f in lib_raw if f.startswith("-l")]

        major = int(ver.split(".")[0])
        if major >= 2:
            sources = ["src/io/export_3mf_v2.cc", "src/io/import_3mf_v2.cc"]
            print(f"lib3mf: found v{ver} (v2 API)")
        else:
            sources = ["src/io/export_3mf_v1.cc", "src/io/import_3mf_v1.cc"]
            print(f"lib3mf: found v{ver} (v1 API)")

        defines = [("ENABLE_LIB3MF", "1")]
        return sources, inc_dirs, libraries, defines

    print("lib3mf: not found, using dummy stubs")
    sources = ["src/io/export_3mf_dummy.cc", "src/io/import_3mf_dummy.cc"]
    return sources, [], [], []


def detect_libfive():
    """Detect libfive submodule and return (sources, include_dirs, defines) or empty fallback."""
    libfive_include = os.path.join("submodules", "libfive", "libfive", "include")
    libfive_src = os.path.join("submodules", "libfive", "libfive", "src")

    if not os.path.isdir(libfive_src):
        print("libfive: submodule not found, skipping")
        return [], [], []

    src_prefix = libfive_src + os.sep
    sources = [
        src_prefix + "eval/base.cpp",
        src_prefix + "eval/deck.cpp",
        src_prefix + "eval/eval_interval.cpp",
        src_prefix + "eval/eval_jacobian.cpp",
        src_prefix + "eval/eval_array.cpp",
        src_prefix + "eval/eval_deriv_array.cpp",
        src_prefix + "eval/eval_feature.cpp",
        src_prefix + "eval/tape.cpp",
        src_prefix + "eval/feature.cpp",
        src_prefix + "render/discrete/heightmap.cpp",
        src_prefix + "render/discrete/voxels.cpp",
        src_prefix + "render/brep/contours.cpp",
        src_prefix + "render/brep/edge_tables.cpp",
        src_prefix + "render/brep/manifold_tables.cpp",
        src_prefix + "render/brep/mesh.cpp",
        src_prefix + "render/brep/neighbor_tables.cpp",
        src_prefix + "render/brep/progress.cpp",
        src_prefix + "render/brep/dc/marching.cpp",
        src_prefix + "render/brep/dc/dc_contourer.cpp",
        src_prefix + "render/brep/dc/dc_mesher.cpp",
        src_prefix + "render/brep/dc/dc_neighbors2.cpp",
        src_prefix + "render/brep/dc/dc_neighbors3.cpp",
        src_prefix + "render/brep/dc/dc_worker_pool2.cpp",
        src_prefix + "render/brep/dc/dc_worker_pool3.cpp",
        src_prefix + "render/brep/dc/dc_tree2.cpp",
        src_prefix + "render/brep/dc/dc_tree3.cpp",
        src_prefix + "render/brep/dc/dc_xtree2.cpp",
        src_prefix + "render/brep/dc/dc_xtree3.cpp",
        src_prefix + "render/brep/dc/dc_object_pool2.cpp",
        src_prefix + "render/brep/dc/dc_object_pool3.cpp",
        src_prefix + "render/brep/hybrid/hybrid_debug.cpp",
        src_prefix + "render/brep/hybrid/hybrid_worker_pool2.cpp",
        src_prefix + "render/brep/hybrid/hybrid_worker_pool3.cpp",
        src_prefix + "render/brep/hybrid/hybrid_neighbors2.cpp",
        src_prefix + "render/brep/hybrid/hybrid_neighbors3.cpp",
        src_prefix + "render/brep/hybrid/hybrid_tree2.cpp",
        src_prefix + "render/brep/hybrid/hybrid_tree3.cpp",
        src_prefix + "render/brep/hybrid/hybrid_xtree2.cpp",
        src_prefix + "render/brep/hybrid/hybrid_xtree3.cpp",
        src_prefix + "render/brep/hybrid/hybrid_object_pool2.cpp",
        src_prefix + "render/brep/hybrid/hybrid_object_pool3.cpp",
        src_prefix + "render/brep/hybrid/hybrid_mesher.cpp",
        src_prefix + "render/brep/simplex/simplex_debug.cpp",
        src_prefix + "render/brep/simplex/simplex_neighbors2.cpp",
        src_prefix + "render/brep/simplex/simplex_neighbors3.cpp",
        src_prefix + "render/brep/simplex/simplex_worker_pool2.cpp",
        src_prefix + "render/brep/simplex/simplex_worker_pool3.cpp",
        src_prefix + "render/brep/simplex/simplex_tree2.cpp",
        src_prefix + "render/brep/simplex/simplex_tree3.cpp",
        src_prefix + "render/brep/simplex/simplex_xtree2.cpp",
        src_prefix + "render/brep/simplex/simplex_xtree3.cpp",
        src_prefix + "render/brep/simplex/simplex_object_pool2.cpp",
        src_prefix + "render/brep/simplex/simplex_object_pool3.cpp",
        src_prefix + "render/brep/simplex/simplex_mesher.cpp",
        src_prefix + "render/brep/vol/vol_neighbors.cpp",
        src_prefix + "render/brep/vol/vol_object_pool.cpp",
        src_prefix + "render/brep/vol/vol_tree.cpp",
        src_prefix + "render/brep/vol/vol_worker_pool.cpp",
        src_prefix + "solve/solver.cpp",
        src_prefix + "tree/opcode.cpp",
        src_prefix + "tree/archive.cpp",
        src_prefix + "tree/data.cpp",
        src_prefix + "tree/deserializer.cpp",
        src_prefix + "tree/serializer.cpp",
        src_prefix + "tree/tree.cpp",
        src_prefix + "tree/operations.cpp",
        src_prefix + "oracle/oracle_clause.cpp",
        src_prefix + "oracle/transformed_oracle.cpp",
        src_prefix + "oracle/transformed_oracle_clause.cpp",
        src_prefix + "libfive.cpp",
    ]

    stdlib_dir = os.path.join("submodules", "libfive", "libfive", "stdlib")
    if os.path.isdir(stdlib_dir):
        sources.append(os.path.join(stdlib_dir, "stdlib.cpp"))
        sources.append(os.path.join(stdlib_dir, "stdlib_impl.cpp"))

    sources.append("src/python/FrepNode.cc")

    include_dirs = [libfive_include]
    defines = [
        ("ENABLE_LIBFIVE", "1"),
        ("GIT_TAG", '"N/A"'),
        ("GIT_REV", '"N/A"'),
        ("GIT_BRANCH", '"N/A"'),
    ]

    print(f"libfive: enabled ({len(sources)} source files)")
    return sources, include_dirs, defines


class BuildExtWithLexYacc(build_ext):
    """Custom build_ext command to run lex/yacc before building extension modules."""

    def run(self):
        yacc_src = "src/core/parser.y"
        lex_src = "src/core/lexer.l"

        yacc_out = "src/core/parser.cc"
        yacc_hdr = "src/core/parser.tab.h"
        lex_out = "src/core/lexer.cc"

        def needs_rebuild(src, targets):
            if not all(os.path.exists(t) for t in targets):
                return True
            src_time = os.path.getmtime(src)
            target_time = min(os.path.getmtime(t) for t in targets)
            return src_time > target_time

        bison_cmd = "win_bison" if IS_WINDOWS else "bison"
        flex_cmd = "win_flex" if IS_WINDOWS else "flex"

        if needs_rebuild(yacc_src, [yacc_out, yacc_hdr]):
            print(f"Generating Yacc: {yacc_src}")
            subprocess.run([bison_cmd, "-d", "-p", "parser", yacc_src], check=True)
            os.rename("parser.tab.c", yacc_out)
            shutil.copyfile("parser.tab.h", "src/core/parser.hxx")
            os.rename("parser.tab.h", yacc_hdr)
        else:
            print(f"{yacc_src} is up to date")

        if needs_rebuild(lex_src, [lex_out]):
            print(f"Generating Lex: {lex_src}")
            subprocess.run([flex_cmd, "-o", lex_out, lex_src], check=True)
        else:
            print(f"{lex_src} is up to date")

        super().run()


def main():
    apply_wheel_build_env()

    root =  [
              "src/Feature.cc",
              "src/FontCache.cc",
              "src/handle_dep.cc"
            ]

    python =[
              "src/genlang/genlang.cc",
              "src/python/pyfunctions.cc",
              "src/python/py_math.cc",
              "src/python/py_primitives.cc",
              "src/python/py_transform.cc",
              "src/python/py_extrude.cc",
              "src/python/py_csg.cc",
              "src/python/py_analysis.cc",
              "src/python/py_ops.cc",
              "src/python/py_io.cc",
              "src/python/pyconversion.cc",
              "src/python/pydata.cc",
              "src/python/pyopenscad.cc",
              "src/python/pymod.cc",
              "src/python/pip_fixer.cc"
            ]
    geometry = [
              "src/geometry/GeometryEvaluator.cc",
              "src/geometry/rotate_extrude.cc",
              "src/geometry/roof_ss.cc",
              "src/geometry/roof_vd.cc",
              "src/geometry/skin.cc",
              "src/geometry/linear_extrude.cc",
              "src/geometry/cgal/CGALCache.cc",
              "src/geometry/cgal/cgalutils.cc",
              "src/geometry/cgal/cgalutils-kernel.cc",
              "src/geometry/cgal/cgalutils-applyops.cc",
              "src/geometry/cgal/cgalutils-applyops-minkowski.cc",
              "src/geometry/cgal/cgalutils-mesh.cc",
              "src/geometry/cgal/cgalutils-triangulate.cc",
              "src/geometry/cgal/cgalutils-tess.cc",
              "src/geometry/cgal/cgalutils-orient.cc",
              "src/geometry/cgal/CGALNefGeometry.cc",
              "src/geometry/cgal/cgalutils-polyhedron.cc",
              "src/geometry/cgal/cgalutils-convex.cc",
              "src/geometry/cgal/cgalutils-project.cc",
              "src/geometry/cgal/cgalutils-closed.cc",
              "src/geometry/Geometry.cc",
              "src/geometry/GeometryCache.cc",
              "src/geometry/GeometryUtils.cc",
              "src/geometry/Polygon2d.cc",
              "src/geometry/Barcode1d.cc",
              "src/geometry/PolySet.cc",
              "src/geometry/PolySetBuilder.cc",
              "src/geometry/PolySetUtils.cc",
              "src/geometry/Surface.cc",
              "src/geometry/Curve.cc",
              "src/geometry/ClipperUtils.cc",
              "src/geometry/linalg.cc",
              "src/geometry/manifold/ManifoldGeometry.cc",
              "src/geometry/manifold/manifold-applyops.cc",
              "src/geometry/manifold/manifoldutils.cc",
              "src/geometry/manifold/manifold-applyops-minkowski.cc",
              "src/geometry/boolean_utils.cc" ]
    ext= [
              "src/ext/libtess2/Source/bucketalloc.c",
              "src/ext/libtess2/Source/sweep.c",
              "src/ext/libtess2/Source/mesh.c",
              "src/ext/libtess2/Source/dict.c",
              "src/ext/libtess2/Source/tess.c",
              "src/ext/libtess2/Source/geom.c",
              "src/ext/libtess2/Source/priorityq.c" ]
    nodes = [
              "src/core/primitives.cc",
              "src/core/CgalAdvNode.cc",
              "src/core/FilletNode.cc",
              "src/core/ProjectionNode.cc",
              "src/core/SurfaceNode.cc",
              "src/core/ColorNode.cc",
              "src/core/ImportNode.cc",
              "src/core/PullNode.cc",
              "src/core/TextNode.cc",
              "src/core/SheetNode.cc",
              "src/core/LinearExtrudeNode.cc",
              "src/core/RotateExtrudeNode.cc",
              "src/core/RenderNode.cc",
              "src/core/CSGNode.cc",
              "src/core/OffsetNode.cc",
              "src/core/RoofNode.cc",
              "src/core/TransformNode.cc",
              "src/core/CsgOpNode.cc",
              "src/core/OversampleNode.cc",
              "src/core/WrapNode.cc",
              "src/core/DebugNode.cc",
              "src/core/PathExtrudeNode.cc",
              "src/core/GroupModule.cc",
              "src/core/SkinNode.cc",
              "src/core/RepairNode.cc"
            ]
    context = [
              "src/core/ContextFrame.cc",
              "src/core/ScopeContext.cc",
              "src/core/LocalScope.cc",
              "src/core/Context.cc",
              "src/core/ContextMemoryManager.cc",
              "src/core/BuiltinContext.cc",
              "src/core/EvaluationSession.cc",
              "src/core/Parameters.cc",
              "src/core/SourceFileCache.cc",
            ]
    arith = [
              "src/core/FunctionType.cc",
              "src/core/UndefType.cc",
              "src/core/Value.cc",
              "src/core/Assignment.cc",
              "src/core/Arguments.cc",
              "src/core/Expression.cc",
              "src/core/SourceFile.cc",
            ]
    language = [
              "src/core/ModuleInstantiation.cc",
              "src/core/UserModule.cc",
              "src/core/module.cc",
              "src/core/Children.cc",
              "src/core/AST.cc",
              "src/core/Builtins.cc",
              "src/core/builtin_functions.cc",
              "src/core/control.cc" ,
              "src/core/function.cc"
            ]
    core = [
              "src/core/CurveDiscretizer.cc",
              "src/core/FreetypeRenderer.cc",
              "src/core/DrawingCallback.cc",
              "src/core/customizer/Annotation.cc",
              "src/core/node.cc",
              "src/core/node_clone.cc",
              "src/core/progress.cc",
              "src/core/parsersettings.cc",
              "src/core/NodeVisitor.cc",
              "src/core/Settings.cc",
              "src/core/Tree.cc",
              "src/core/ColorUtil.cc",
              "src/core/NodeDumper.cc",
              "src/core/StatCache.cc",
              ]  + language + arith + context + nodes
    io_export = [
              "src/io/export_stl.cc",
              "src/io/export_dxf.cc",
              "src/io/export_off.cc",
              "src/io/export_pov.cc",
              "src/io/export_svg.cc",
              "src/io/export_gcode.cc",
              "src/io/export_foldable.cc",
              "src/io/export_ps.cc",
              "src/io/export_wrl.cc",
              "src/io/export_amf.cc",
              "src/io/export_nef.cc",
              "src/io/export_pdf.cc",
              "src/io/export_obj.cc",
              "src/io/export_step.cc"
            ]
    io_import = [
              "src/io/import_json.cc",
              "src/io/import_obj.cc",
              "src/io/import_step.cc",
              "src/io/import_amf.cc",
              "src/io/import_nef.cc",
              "src/io/import_svg.cc",
              "src/io/import_off.cc",
              "src/io/import_stl.cc" ]
    libsvg = [
              "src/libsvg/circle.cc",
              "src/libsvg/line.cc",
              "src/libsvg/shape.cc",
              "src/libsvg/use.cc",
              "src/libsvg/data.cc",
              "src/libsvg/path.cc",
              "src/libsvg/svgpage.cc",
              "src/libsvg/util.cc",
              "src/libsvg/ellipse.cc",
              "src/libsvg/polygon.cc",
              "src/libsvg/text.cc",
              "src/libsvg/group.cc",
              "src/libsvg/polyline.cc",
              "src/libsvg/transformation.cc",
              "src/libsvg/libsvg.cc",
              "src/libsvg/rect.cc",
              "src/libsvg/tspan.cc"
              ]
    io = [
              "src/io/export.cc",
              "src/io/DxfData.cc",
              "src/io/fileutils.cc",
              "src/io/StepKernel.cc",
              "src/io/dxfdim.cc" ] + io_export + io_import
    manifold = [
              "submodules/manifold/src/manifold.cpp",
              "submodules/manifold/src/quickhull.cpp",
              "submodules/manifold/src/boolean_result.cpp",
              "submodules/manifold/src/cross_section/cross_section.cpp",
              "submodules/manifold/src/constructors.cpp",
              "submodules/manifold/src/smoothing.cpp",
              "submodules/manifold/src/subdivision.cpp",
              "submodules/manifold/src/properties.cpp",
              "submodules/manifold/src/face_op.cpp",
              "submodules/manifold/src/boolean3.cpp",
              "submodules/manifold/src/edge_op.cpp",
              "submodules/manifold/src/csg_tree.cpp",
              "submodules/manifold/src/sort.cpp",
              "submodules/manifold/src/sdf.cpp",
              "submodules/manifold/src/polygon.cpp",
              "submodules/manifold/src/tree2d.cpp",
              "submodules/manifold/src/minkowski.cpp",
              "submodules/manifold/src/impl.cpp" ]
    clipper = [
              "submodules/Clipper2/CPP/Clipper2Lib/src/clipper.engine.cpp",
              "submodules/Clipper2/CPP/Clipper2Lib/src/clipper.offset.cpp",
              "submodules/Clipper2/CPP/Clipper2Lib/src/clipper.rectclip.cpp" ]
    utils = [
              "src/utils/printutils.cc",
              "src/utils/degree_trig.cc",
              "src/utils/hash.cc",
              "src/utils/svg.cc",
              "src/utils/vector_math.cc",
              "src/utils/png_util.cc",
              "src/utils/calc.cc" ]
    platform = get_platform_sources()
    glview = [
              "src/glview/Camera.cc",
#              "src/glview/PolySetRenderer.cc",
              "src/glview/ColorMap.cc",
              "src/glview/RenderSettings.cc" ]
    lex_yacc = [
              "src/core/parser.cc",
              "src/core/lexer.cc"
              ]
    lodepng = [ "src/ext/lodepng/lodepng.cpp" ]

    # Detect optional dependencies
    lib3mf_sources, lib3mf_incdirs, lib3mf_libs, lib3mf_defines = detect_lib3mf()
    libfive_sources, libfive_incdirs, libfive_defines = detect_libfive()

    project_include_dirs = [
        ".",
        "src",
        "src/core",
        "src/ext",
        "src/geometry",
        "src/io",
        "src/utils",
        "src/platform",
        "src/ext/libtess2/Include",
        "submodules/Clipper2/CPP/Clipper2Lib/include",
        "submodules/manifold/include",
    ] + get_pkg_config_include_dirs() + lib3mf_incdirs + libfive_incdirs

    all_sources = (root + python + geometry + ext + io + lib3mf_sources +
                   libsvg + core + manifold + clipper + utils + platform +
                   glview + lex_yacc + lodepng + libfive_sources)
    version, shortversion, version_major, version_minor, version_patch = get_version_parts()

    all_defines = [
        ("ENABLE_PYTHON", "1"),
        ("ENABLE_CGAL", "1"),
        ("ENABLE_MANIFOLD", "1"),
        ("EXPERIMENTAL", "1"),
        ("OPENSCAD_NOGUI", "1"),
        ("PYTHON_EXECUTABLE_NAME", '"pythonscad"'),
        ("MANIFOLD_PAR", "-1"),
        ("OPENSCAD_VERSION", version),
        ("OPENSCAD_SHORTVERSION", shortversion),
        ("OPENSCAD_MAJOR", version_major),
        ("OPENSCAD_MINOR", version_minor),
        ("OPENSCAD_PATCH", version_patch),
        ("OPENSCAD_YEAR", version_major),
        ("OPENSCAD_MONTH", version_minor),
        ("OPENSCAD_DAY", version_patch),
        ("STACKSIZE", "524288"),
    ] + lib3mf_defines + libfive_defines + get_extra_defines()

    pythonscad_ext = Extension(
        "_openscad",
        sources=all_sources,
        include_dirs=project_include_dirs,
        library_dirs=get_library_dirs(),
        libraries=get_link_libraries() + lib3mf_libs,
        define_macros=all_defines,
        extra_compile_args=get_extra_compile_args(),
        extra_link_args=get_extra_link_args(),
    )

    # The pure-Python overlay packages live under libraries/python/{openscad,pythonscad}
    # so they are already shipped alongside the binary by the standalone CMake install
    # (the libraries/ tree). Declare them here so `pip install` exposes the same
    # three-module layout (`_openscad` + `openscad` + `pythonscad`) on PyPI.
    setup(
        version=get_version(),
        cmdclass={"build_ext": BuildExtWithLexYacc},
        ext_modules=[pythonscad_ext],
        packages=["openscad", "pythonscad"],
        package_dir={
            "openscad": "libraries/python/openscad",
            "pythonscad": "libraries/python/pythonscad",
        },
    )


if __name__ == "__main__":
    main()
