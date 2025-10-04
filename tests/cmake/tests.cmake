# Types of tests:
# o echo: Just record console output
# o dump: Export .csg
# o render-cgal: Export to PNG using --render
# o render-force-cgal: Export to PNG using --render=force
# o render-manifold: Export to PNG using --render with --backend=manifold
# o render-force-manifold: Export to PNG using --render=force with --backend=manifold
# o preview-cgal: Export to PNG using OpenCSG
# o preview-manifold: Export to PNG in preview mode with --backend=manifold
# o throwntogether-cgal: Export to PNG using the Throwntogether renderer
# o throwntogether-manifold
# o render-csg-cgal: 1) Export to .csg, 2) import .csg and export to PNG (--render)
# o render-monotone: Same as render-cgal but with the "Monotone" color scheme
# o preview-stl: Export to STL, Re-import and render to PNG (--render)
# o render-stl: Export to STL, Re-import and render to PNG (--render=force)
# o preview-off: Export to OFF, Re-import and render to PNG (--render)
# o render-off: Export to STL, Re-import and render to PNG (--render=force)
# o render-dxf: Export to DXF, Re-import and render to PNG (--render=force)
#

include(./cmake/tests/ast.cmake)
include(./cmake/tests/echo.cmake)
include(./cmake/tests/render.cmake)
include(./cmake/tests/pov.cmake)
include(./cmake/tests/dxf.cmake)
include(./cmake/tests/stl.cmake)
include(./cmake/tests/obj.cmake)
include(./cmake/tests/off.cmake)
include(./cmake/tests/amf.cmake)
include(./cmake/tests/3mf.cmake)
include(./cmake/tests/pdf.cmake)
include(./cmake/tests/svg.cmake)
include(./cmake/tests/expected_fail.cmake)
include(./cmake/tests/customizer.cmake)
include(./cmake/tests/camera.cmake)
include(./cmake/tests/view_options.cmake)
include(./cmake/tests/colorscheme.cmake)
include(./cmake/tests/experimental.cmake)
include(./cmake/tests/relative_filenames.cmake)

add_cmdline_test(csgterm      OPENSCAD SUFFIX term FILES
  ${TEST_SCAD_DIR}/misc/allexpressions.scad
  ${TEST_SCAD_DIR}/misc/allfunctions.scad
  ${TEST_SCAD_DIR}/misc/allmodules.scad
)

add_cmdline_test(dump           OPENSCAD FILES ${FEATURES_2D_FILES} ${FEATURES_3D_FILES} ${DEPRECATED_3D_FILES} ${MISC_FILES} SUFFIX csg ARGS)
add_cmdline_test(dump-examples  OPENSCAD FILES ${EXAMPLE_FILES} SUFFIX csg ARGS)
# non-ASCII filenames
add_cmdline_test(export-csg-nonascii  OPENSCAD FILES ${TEST_SCAD_DIR}/misc/sfære.scad SUFFIX csg)

# 
# OBJECT echo tests
#

file(GLOB OBJECT_TEST ${TEST_SCAD_DIR}/experimental/object/*.scad)
add_cmdline_test(echo EXPERIMENTAL OPENSCAD SUFFIX echo FILES ${OBJECT_TEST} ARGS --enable object-function)

#
# There are some caveats with export and import, so we need to test a few combinations:
# 1. It may be possible to export a non-manifold mesh (e.g. malformed polyhedron) due to 
#    no manifoldness checks at export time. This is by design, e.g. to allow users to 
#    troubleshoot externally.
# 2. It may be possible to import such non-manifolds and preview or render them, but it will
#    likely fail when trying to construct a data structure requiring manifold objects (e.g. --render=force)
#
# This leads to three types of tests:
# 1. <format>render: Manifold object export + render (both with --render=force)
# 2. <format>preview: Non-manifold polyhedron export + preview
# 3. <format>render: Complex Manifold polyhedron export + render (both with --render=force)
#    e.g. self-touching polyhedron or malformed but repairable.
#

# Export-import tests
add_cmdline_test(render-monotone OPENSCAD SUFFIX png FILES ${EXPORT_IMPORT_3D_PREVIEW_FILES} ${SIMPLE_EXPORT_IMPORT_2D_FILES} ARGS --colorscheme=Monotone --render)

# Verify that test framework is paying attention to alpha channel, issue 1492
#add_cmdline_test(openscad-colorscheme-cornfield-alphafail  ARGS --colorscheme=Cornfield SUFFIX png FILES ${EXAMPLES_DIR}/Basics/logo.scad)

# The "expected image" supplied for this "alphafail" test has the alpha channel for all background pixels cleared (a==0),
# when they should be opaque (a==1) for this colorscheme.
# So if test framework is functioning properly then the image comparison should fail.
# Commented out because the master branch isn't capable of making the expected image yet.
# Also TEST_GENERATE=1 makes an expected image that makes the test fail.
#set_property(TEST openscad-colorscheme-cornfield-alphafail_logo PROPERTY WILL_FAIL TRUE)
