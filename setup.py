from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import subprocess
import os
import shutil

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

        if needs_rebuild(yacc_src, [yacc_out, yacc_hdr]):
            print(f"→ Generiere Yacc: {yacc_src}")
            subprocess.run(["bison", "-d", "-p", "parser", yacc_src], check=True)
            os.rename("parser.tab.c", yacc_out)
            shutil.copyfile("parser.tab.h", "src/core/parser.hxx")
            os.rename("parser.tab.h", yacc_hdr)
        else:
            print(f"✓ {yacc_src} is recent")

        if needs_rebuild(lex_src, [lex_out]):
            print(f"→ Generiere Lex: {lex_src}")
            subprocess.run(["lex",  "-o", lex_out,  lex_src], check=True)
        else:
            print(f"✓ {lex_src} is recent")

        super().run()


def main():
    root =  [
              "src/Feature.cc",
              "src/FontCache.cc",
              "src/handle_dep.cc"
            ]

    python =[
              "src/genlang/genlang.cc",
              "src/python/pyfunctions.cc",
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
              "src/core/ConcatNode.cc",
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
              "src/io/export_3mf_dummy.cc",
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
              "src/io/import_3mf_dummy.cc",
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
              "submodules/manifold/src/lazy_collider.cpp",
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
              "src/utils/calc.cc" ]
    platform = [
              "src/platform/PlatformUtils.cc",
              "src/platform/PlatformUtils-posix.cc"
            ]
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

    pythonscad_ext = Extension("openscad"
        , sources = root + python + geometry + ext + io + libsvg + core +  manifold +
        clipper + utils + platform  + glview + lex_yacc + lodepng
        ,include_dirs = [
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
                  "/usr/include/eigen3",
                  "/usr/include/harfbuzz",
                  "/usr/include/libxml2",
                  "/usr/include/freetype2",
                  "/usr/include/glib-2.0",
                  "/usr/include/cairo",
                  "/usr/lib64/glib-2.0/include",
                  "/usr/lib/x86_64-linux-gnu/glib-2.0/include"
                ],libraries=[
                  "freetype",
                  "xml2",
                  "fontconfig",
                  "double-conversion",
                  "gmp",
                  "harfbuzz",
                  "mpfr"
                ],define_macros=[
                  ("ENABLE_PYTHON","1"),
                  ("ENABLE_CGAL","1"),
                  ("ENABLE_MANIFOLD","1"),
                  ("EXPERIMENTAL","1"),
                  ("OPENSCAD_NOGUI","1"),
                  ("PYTHON_EXECUTABLE_NAME","\"pythonscad\""),
                  ("MANIFOLD_PAR","-1"),
                  ("OPENSCAD_YEAR","2025"),
                  ("OPENSCAD_MONTH","2"),
                  ("STACKSIZE","524288")
                ],undef_macros=[
                ], extra_link_args=[
                        "-l", "glib-2.0"
                ],  extra_compile_args=[
                ])

    setup(name="pythonscad",
          version="2025.10.31",
          description="Python interface to openscad",
          url="https://pythonscad.org",
          author="Guenther Sohler",
          author_email="guenther.sohler@gmail.com",
          license="MIT",
          classifiers=[
            "Programming Language :: Python :: 3",
            "Programming Language :: Python :: 3.11" ],
          cmdclass={"build_ext": BuildExtWithLexYacc},
          ext_modules=[ pythonscad_ext ]
          )

if __name__ == "__main__":
    main()
