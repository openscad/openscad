from distutils.core import setup, Extension

def main():
    root =  [
              "../Feature.cc",
              "../FontCache.cc",
              "../version.cc",
              "../handle_dep.cc" 
            ]

    python =[
              "pyfunctions.cc",
              "pydata.cc",
              "pyopenscad.cc",
              "pip_fixer.cc"
            ] 
    geometry = [
              "../geometry/GeometryEvaluator.cc",
              "../geometry/rotextrude.cc",
              "../geometry/skin.cc",
              "../geometry/linear_extrude.cc",
              "../geometry/cgal/CGALCache.cc",
              "../geometry/cgal/cgalutils.cc",
              "../geometry/cgal/cgalutils-kernel.cc",
              "../geometry/cgal/cgalutils-applyops.cc",
              "../geometry/cgal/cgalutils-mesh.cc",
              "../geometry/cgal/cgalutils-triangulate.cc",
              "../geometry/cgal/cgalutils-tess.cc",
              "../geometry/cgal/cgalutils-orient.cc",
              "../geometry/cgal/cgalutils-mesh.cc",
              "../geometry/cgal/CGAL_Nef_polyhedron.cc",
              "../geometry/cgal/cgalutils-polyhedron.cc",
              "../geometry/cgal/cgalutils-convex.cc",
              "../geometry/cgal/cgalutils-project.cc",
              "../geometry/cgal/cgalutils-closed.cc",
              "../geometry/Geometry.cc",
              "../geometry/GeometryCache.cc",
              "../geometry/GeometryUtils.cc",
              "../geometry/Polygon2d.cc",
              "../geometry/PolySet.cc",
              "../geometry/PolySetBuilder.cc",
              "../geometry/PolySetUtils.cc",
              "../geometry/Surface.cc",
              "../geometry/Curve.cc",
              "../geometry/ClipperUtils.cc",
              "../geometry/linalg.cc",
              "../geometry/manifold/ManifoldGeometry.cc",
              "../geometry/manifold/manifold-applyops.cc",
              "../geometry/manifold/manifoldutils.cc",
              "../geometry/manifold/manifold-applyops-minkowski.cc",
              "../geometry/boolean_utils.cc" ]
    ext= [
              "../ext/libtess2/Source/bucketalloc.c",
              "../ext/libtess2/Source/sweep.c",
              "../ext/libtess2/Source/mesh.c",
              "../ext/libtess2/Source/dict.c",
              "../ext/libtess2/Source/tess.c",
              "../ext/libtess2/Source/geom.c",
              "../ext/libtess2/Source/priorityq.c" ]
    nodes = [
              "../core/primitives.cc",
              "../core/CgalAdvNode.cc",
              "../core/FilletNode.cc",
              "../core/ProjectionNode.cc",
              "../core/SurfaceNode.cc",
              "../core/ColorNode.cc",
              "../core/ImportNode.cc",
              "../core/PullNode.cc",
              "../core/TextNode.cc",
              "../core/ConcatNode.cc",
              "../core/LinearExtrudeNode.cc",
              "../core/RotateExtrudeNode.cc",
              "../core/RenderNode.cc",
              "../core/TextureNode.cc",
              "../core/CSGNode.cc",
              "../core/OffsetNode.cc",
              "../core/RoofNode.cc",
              "../core/TransformNode.cc",
              "../core/CsgOpNode.cc",
              "../core/OversampleNode.cc",
              "../core/WrapNode.cc",
              "../core/DebugNode.cc",
              "../core/PathExtrudeNode.cc",
              "../core/GroupModule.cc",
              "../core/SkinNode.cc"
            ]
    context = [
              "../core/ContextFrame.cc",
              "../core/ScopeContext.cc",
              "../core/LocalScope.cc",
              "../core/Context.cc",
              "../core/ContextMemoryManager.cc",
              "../core/BuiltinContext.cc",
              "../core/EvaluationSession.cc",
              "../core/Parameters.cc",
              "../core/SourceFileCache.cc",
            ]
    arith = [
              "../core/FunctionType.cc",
              "../core/UndefType.cc",
              "../core/Value.cc",
              "../core/Assignment.cc",
              "../core/Arguments.cc",
              "../core/Expression.cc",
              "../core/SourceFile.cc",
            ]
    language = [
              "../core/ModuleInstantiation.cc",
              "../core/UserModule.cc",
              "../core/module.cc",
              "../core/Children.cc",
              "../core/AST.cc",
              "../core/Builtins.cc",
              "../core/builtin_functions.cc",
              "../core/control.cc" ,
              "../core/function.cc"
            ]
    core = [
              "../core/FreetypeRenderer.cc",
              "../core/DrawingCallback.cc",
              "../core/customizer/Annotation.cc",
              "../core/node.cc",
              "../core/node_clone.cc",
              "../core/progress.cc",
              "../core/parsersettings.cc",
              "../core/NodeVisitor.cc",
              "../core/Settings.cc",
              "../core/Tree.cc",
              "../core/ColorUtil.cc",
              "../core/NodeDumper.cc",
              "../core/StatCache.cc",
              ]  + language + arith + context + nodes
    io_export = [
              "../io/export_stl.cc",
              "../io/export_dxf.cc",
              "../io/export_off.cc",
              "../io/export_pov.cc",
              "../io/export_svg.cc",
              "../io/export_foldable.cc",
              "../io/export_ps.cc",
              "../io/export_wrl.cc",
              "../io/export_amf.cc",
              "../io/export_nef.cc",
              "../io/export_pdf.cc",
              "../io/export_obj.cc",
              "../io/export_step.cc"
            ]
    io_import = [            
              "../io/import_json.cc",
              "../io/import_obj.cc",
              "../io/import_step.cc",
              "../io/import_3mf_dummy.cc",
              "../io/import_amf.cc",
              "../io/import_nef.cc",
              "../io/import_off.cc",
              "../io/import_stl.cc" ]
    io = [              
              "../io/export.cc",
              "../io/DxfData.cc",
              "../io/fileutils.cc",
              "../io/StepKernel.cc",
              "../io/dxfdim.cc" ] + io_export + io_import
    manifold = [
              "../../submodules/manifold/src/manifold.cpp",
              "../../submodules/manifold/src/quickhull.cpp",
              "../../submodules/manifold/src/boolean_result.cpp",
              "../../submodules/manifold/src/cross_section/cross_section.cpp",
              "../../submodules/manifold/src/constructors.cpp",
              "../../submodules/manifold/src/smoothing.cpp",
              "../../submodules/manifold/src/subdivision.cpp",
              "../../submodules/manifold/src/properties.cpp",
              "../../submodules/manifold/src/face_op.cpp",
              "../../submodules/manifold/src/boolean3.cpp",
              "../../submodules/manifold/src/edge_op.cpp",
              "../../submodules/manifold/src/csg_tree.cpp",
              "../../submodules/manifold/src/sort.cpp",
              "../../submodules/manifold/src/sdf.cpp",
              "../../submodules/manifold/src/polygon.cpp",
              "../../submodules/manifold/src/impl.cpp" ]
    clipper = [
              "../../submodules/Clipper2/CPP/Clipper2Lib/src/clipper.engine.cpp",
              "../../submodules/Clipper2/CPP/Clipper2Lib/src/clipper.offset.cpp",
              "../../submodules/Clipper2/CPP/Clipper2Lib/src/clipper.rectclip.cpp" ]
    utils = [
              "../utils/printutils.cc",
              "../utils/degree_trig.cc",
              "../utils/hash.cc",
              "../utils/svg.cc",
              "../utils/calc.cc" ]
    platform = [
              "../platform/PlatformUtils.cc",
              "../platform/PlatformUtils-posix.cc"
            ]
    glview = [
              "../glview/Camera.cc",
              "../glview/ColorMap.cc",
              "../glview/RenderSettings.cc" ]
    lex_yacc = [
              "../../build/objects/parser.cxx",
              "../../build/objects/lexer.cxx"
              ]
    lodepng = [ "../ext/lodepng/lodepng.cpp" ]

    pythonscad_ext = Extension("openscad"
        , sources = python + geometry + ext + io + core +  manifold + 
        clipper + utils + platform  + glview + lex_yacc + lodepng
        ,include_dirs = [
                  "../..",
                  "../../src",
                  "../../src/core",
                  "../../src/ext",
                  "../../src/geometry",
                  "../../src/io",
                  "../../src/utils",
                  "../../src/platform",
                  "../../src/ext/libtess2/Include",
                  "../../submodules/Clipper2/CPP/Clipper2Lib/include",
                  "../../submodules/manifold/include",
                  "/usr/include/eigen3",
                  "/usr/include/harfbuzz",
                  "/usr/include/libxml2",
                  "/usr/include/freetype2",
                  "/usr/include/glib-2.0",
                  "/usr/include/cairo",
                  "/usr/lib64/glib-2.0/include"
                ],libraries=[
                  "freetype",
                  "jpeg",
                  "xml2",
                  "fontconfig",
                  "double-conversion",
                  "gmp",
                  "mpfr"
                ],define_macros=[
                  ("ENABLE_PYTHON","1"),
                  ("ENABLE_CGAL","1"),
                  ("ENABLE_MANIFOLD","1"),
                  ("ENABLE_PIP","1"),
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
          version="2025.02",
          description="Python interface to openscad",
          url="https://pythonscad.org",
          author="Guenther Sohler",
          author_email="guenther.sohler@gmail.com",
          license="MIT",
          classifiers=[
            "Programming Language :: Python :: 3",
            "Programming Language :: Python :: 3.11" ],
          ext_modules=[ pythonscad_ext ]
          )

if __name__ == "__main__":
    main()


'''
'''
