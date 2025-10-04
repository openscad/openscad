list(APPEND RENDERFORCETEST_FILES
  ${TEST_SCAD_DIR}/3D/issues/issue5548.scad
  ${TEST_SCAD_DIR}/3D/issues/issue5135-good.scad
  ${TEST_SCAD_DIR}/3D/issues/issue5135-bad.scad
  ${TEST_SCAD_DIR}/3D/misc/import-single-triangle.scad
)

# Preview in CGAL and Manifold should be the same, except for objects with color
# ..but there are some exceptions.
list(APPEND PREVIEW_DIFFERENT_EXPECTATIONS
  # Manifold can construct geometry which isn't representable in CGAL mode,
  # causing CGAL-based operations like minkowski() to fail.
  ${TEST_SCAD_DIR}/3D/issues/issue1137.scad

  # render() in manifold mode preserves colors
  ${TEST_SCAD_DIR}/3D/features/render-tests.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity.scad
  ${TEST_SCAD_DIR}/3D/features/render-preserve-colors.scad

  # resize() in manifold mode preserves colors
  ${TEST_SCAD_DIR}/3D/features/resize-convexity-tests.scad

  # TODO(kintel): Run bugs files
  # ${TEST_SCAD_DIR}/bugs/issue1000.scad
  # ${TEST_SCAD_DIR}/bugs/issue802.scad
  # ${TEST_SCAD_DIR}/bugs2D/issue2220.scad
)

# only added to RENDER_DIFFERENT_EXPECTATIONS
list(APPEND SCADFILES_WITH_COLOR
  ${TEST_SCAD_DIR}/3D/features/child-child-test.scad
  ${TEST_SCAD_DIR}/3D/features/color-tests.scad
  ${TEST_SCAD_DIR}/3D/features/color-tests2.scad
  ${TEST_SCAD_DIR}/3D/features/color-tests3.scad
  ${TEST_SCAD_DIR}/3D/features/render-preserve-colors.scad
  ${TEST_SCAD_DIR}/3D/features/highlight-modifier.scad
  ${TEST_SCAD_DIR}/3D/features/linear_extrude-parameter-tests.scad
  ${TEST_SCAD_DIR}/3D/features/rotate-parameters.scad
  ${TEST_SCAD_DIR}/3D/features/resize-tests.scad
  ${TEST_SCAD_DIR}/3D/features/color-names-tests.scad
  ${TEST_SCAD_DIR}/3D/features/hex-colors-tests.scad

  ${EXAMPLES_DIR}/Advanced/assert.scad
  ${EXAMPLES_DIR}/Advanced/animation.scad
  ${EXAMPLES_DIR}/Advanced/GEB.scad
  ${EXAMPLES_DIR}/Advanced/children.scad
  ${EXAMPLES_DIR}/Advanced/children_indexed.scad
  ${EXAMPLES_DIR}/Basics/CSG-modules.scad
  ${EXAMPLES_DIR}/Basics/linear_extrude.scad
  ${EXAMPLES_DIR}/Basics/logo_and_text.scad
  ${EXAMPLES_DIR}/Basics/projection.scad
  ${EXAMPLES_DIR}/Basics/text_on_cube.scad
  ${EXAMPLES_DIR}/Advanced/surface_image.scad
  ${EXAMPLES_DIR}/Functions/functions.scad
  ${EXAMPLES_DIR}/Basics/rotate_extrude.scad
  ${EXAMPLES_DIR}/Old/example017.scad

  ${TEST_SCAD_DIR}/3D/issues/issue5217.scad
  ${TEST_SCAD_DIR}/3D/issues/issue5738.scad

  #${TEST_SCAD_DIR}/bugs/issue1000.scad
)

# Render in CGAL and Manifold should be the same, except for objects with color
# ..but there are some exceptions.
list(APPEND RENDER_DIFFERENT_EXPECTATIONS
  # Manifold backend supports color rendering
  ${SCADFILES_WITH_COLOR}

  # Manifold can repair winding order
  ${TEST_SCAD_DIR}/3D/features/polyhedron-tests.scad

  # Manifold can construct geometry which isn't representable in CGAL mode,
  # causing CGAL-based operations like minkowski() to fail.
  ${TEST_SCAD_DIR}/3D/issues/issue1137.scad

  # render() in manifold mode preserves colors
  ${TEST_SCAD_DIR}/3D/features/render-tests.scad
  ${EXAMPLES_DIR}/Old/example016.scad

  # Manifold can represent the very thin slice
  ${TEST_SCAD_DIR}/3D/issues/issue1165.scad
)

list(APPEND EXAMPLE_3D_FILES ${EXAMPLE_FILES})
# Remove 2D files from 3D examples
list(REMOVE_ITEM EXAMPLE_3D_FILES
  ${EXAMPLES_DIR}/Old/example015.scad
  ${EXAMPLES_DIR}/Advanced/module_recursion.scad
  ${EXAMPLES_DIR}/Functions/list_comprehensions.scad
  ${EXAMPLES_DIR}/Functions/polygon_areas.scad
  ${EXAMPLES_DIR}/Functions/recursion.scad
)

list(APPEND RENDER_3D_FILES ${FEATURES_3D_FILES} ${SCAD_AMF_FILES} ${DEPRECATED_3D_FILES} ${ISSUES_3D_FILES} ${EXAMPLE_3D_FILES} ${SCAD_NEF3_FILES}
  ${TEST_SCAD_DIR}/misc/include-tests.scad
  ${TEST_SCAD_DIR}/misc/use-tests.scad
  ${TEST_SCAD_DIR}/misc/assert-tests.scad
  ${TEST_SCAD_DIR}/misc/let-module-tests.scad
  ${TEST_SCAD_DIR}/misc/localfiles-test.scad
  ${TEST_SCAD_DIR}/misc/localfiles_dir/localfiles-compatibility-test.scad
  ${TEST_SCAD_DIR}/misc/rotate-empty-bbox.scad
  ${TEST_SCAD_DIR}/misc/empty-shape-tests.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity-polyhedron.scad
  ${TEST_SCAD_DIR}/misc/bad-stl-pcbvicebar.scad
  ${TEST_SCAD_DIR}/misc/bad-stl-tardis.scad
  ${TEST_SCAD_DIR}/misc/bad-stl-wing.scad
  ${TEST_SCAD_DIR}/misc/rotate_extrude-hole.scad
  ${TEST_SCAD_DIR}/misc/preview_variable.scad
)

list(APPEND RENDER_2D_FILES ${FEATURES_2D_FILES} ${SCAD_DXF_FILES} ${ISSUES_2D_FILES} ${EXAMPLE_2D_FILES})

list(GET RENDER_2D_FILES 0 1 2 RENDERSTDIOTEST_FILES)

list(APPEND ALL_RENDER_FILES ${RENDER_2D_FILES} ${RENDER_3D_FILES} ${BUGS_FILES} ${BUGS_2D_FILES})

list(APPEND PREVIEW_ONLY_FILES
  ${TEST_SCAD_DIR}/3D/features/child-background.scad
  ${TEST_SCAD_DIR}/3D/features/highlight-and-background-modifier.scad
  ${TEST_SCAD_DIR}/3D/features/highlight-modifier2.scad
  ${TEST_SCAD_DIR}/3D/features/background-modifier2.scad
  ${TEST_SCAD_DIR}/2D/issues/issue5574.scad
)

list(REMOVE_ITEM ALL_RENDER_FILES 
  # These tests only makes sense in preview mode
  ${PREVIEW_ONLY_FILES}
)

# not sure where this goes
set(PRUNE_TEST ${TEST_SCAD_DIR}/misc/intersection-prune-test.scad)

list(APPEND OBJ_IMPORT_FILES ${TEST_SCAD_DIR}/obj/obj-import-centered.scad)
list(APPEND 3MF_IMPORT_FILES ${TEST_SCAD_DIR}/3mf/3mf-import-centered.scad)
list(APPEND OFF_IMPORT_FILES ${TEST_SCAD_DIR}/off/off-import-centered.scad)
# test importing unparseable files, result will be an empty image
list(APPEND STL_IMPORT_FILES
  ${TEST_SCAD_DIR}/stl/stl-import-invalidvertex.scad
  ${TEST_SCAD_DIR}/stl/stl-import-toomanyvertices.scad
  ${TEST_SCAD_DIR}/stl/stl-import-unparseable.scad
  # result will not be empty
  ${TEST_SCAD_DIR}/stl/stl-import-centered.scad
  ${TEST_SCAD_DIR}/stl/stl-import-not-centered.scad
)

list(APPEND ALL_PREVIEW_FILES ${3MF_IMPORT_FILES} ${OBJ_IMPORT_FILES} ${OFF_IMPORT_FILES} ${STL_IMPORT_FILES} ${ALL_RENDER_FILES} ${PRUNE_TEST} ${PREVIEW_ONLY_FILES})

list(APPEND PREVIEW_COMMON_FILES ${ALL_PREVIEW_FILES})
list(REMOVE_ITEM PREVIEW_COMMON_FILES
  ${PREVIEW_DIFFERENT_EXPECTATIONS}
)

list(APPEND RENDER_COMMON_FILES ${ALL_RENDER_FILES})
list(REMOVE_ITEM RENDER_COMMON_FILES
  ${SCAD_NEF3_FILES}       # Nef3 import not supported in Manifold mode
  ${RENDER_DIFFERENT_EXPECTATIONS}
)

list(APPEND ALL_THROWNTOGETHER_FILES ${ALL_RENDER_FILES})
list(REMOVE_ITEM ALL_THROWNTOGETHER_FILES
  ${PREVIEW_DIFFERENT_EXPECTATIONS}
  ${SCAD_NEF3_FILES}
  ${TEST_SCAD_DIR}/3D/features/color-tests2.scad
  ${TEST_SCAD_DIR}/3D/features/minkowski3-erosion.scad
  ${TEST_SCAD_DIR}/3D/features/nullspace-difference.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity-polyhedron.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1165.scad
	${TEST_SCAD_DIR}/3D/issues/issue1803.scad
	${TEST_SCAD_DIR}/3D/issues/issue2090.scad
	${TEST_SCAD_DIR}/3D/issues/issue2841.scad
	${TEST_SCAD_DIR}/3D/issues/issue2841b.scad
)

# FIXME: We don't actually need to compare the output of cgalstlsanitytest
# with anything. It's self-contained and returns != 0 on error

add_cmdline_test(render-cgal      OPENSCAD FILES ${RENDER_COMMON_FILES} EXPECTEDDIR render SUFFIX png ARGS --render --backend=cgal)
add_cmdline_test(render-cgal      OPENSCAD FILES ${RENDER_DIFFERENT_EXPECTATIONS} SUFFIX png ARGS --render --backend=cgal)
add_cmdline_test(render-force-cgal OPENSCAD SUFFIX png FILES ${RENDERFORCETEST_FILES} ARGS --render=force --backend=cgal)
add_cmdline_test(render-stdio-cgal OPENSCAD SUFFIX png FILES ${RENDERSTDIOTEST_FILES} STDIO EXPECTEDDIR render ARGS --export-format png --render --backend=cgal)

add_cmdline_test(preview-cgal    OPENSCAD FILES ${PREVIEW_COMMON_FILES} EXPECTEDDIR preview SUFFIX png ARGS --backend=cgal)
add_cmdline_test(preview-cgal    OPENSCAD FILES ${PREVIEW_DIFFERENT_EXPECTATIONS} SUFFIX png ARGS --backend=cgal)
add_cmdline_test(throwntogether-cgal  OPENSCAD FILES ${ALL_THROWNTOGETHER_FILES} SUFFIX png EXPECTEDDIR throwntogether ARGS --preview=throwntogether --backend=cgal)

add_cmdline_test(render-csg-cgal SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${RENDER_COMMON_FILES} EXPECTEDDIR render ARGS ${OPENSCAD_EXE_ARG} --format=csg --render --backend=cgal)
add_cmdline_test(render-csg-cgal SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${RENDER_DIFFERENT_EXPECTATIONS} EXPECTEDDIR render-cgal ARGS ${OPENSCAD_EXE_ARG} --format=csg --render --backend=cgal)


list(APPEND MANIFOLDHARDWARNING_FILES
  ${TEST_SCAD_DIR}/3D/features/polyhedron-soup.scad
  ${TEST_SCAD_DIR}/3D/issues/issue5741.scad
  ${TEST_SCAD_DIR}/3D/issues/issue5555.scad
  ${TEST_SCAD_DIR}/3D/issues/issue5555b.scad
  ${TEST_SCAD_DIR}/3D/issues/issue5135.scad
  ${TEST_SCAD_DIR}/3D/issues/issue5135-good.scad
)

if (ENABLE_MANIFOLD)
add_cmdline_test(render-manifold OPENSCAD FILES ${RENDER_COMMON_FILES} EXPECTEDDIR render SUFFIX png ARGS --render --backend=manifold)
add_cmdline_test(render-manifold OPENSCAD FILES ${RENDER_DIFFERENT_EXPECTATIONS} SUFFIX png ARGS --render --backend=manifold)
add_cmdline_test(render-force-manifold      OPENSCAD SUFFIX png FILES ${RENDERFORCETEST_FILES} EXPECTEDDIR render ARGS --render=force --backend=manifold)
# This tests that no warnings are issued when using Manifold for converting or processing geometry
add_cmdline_test(render-force-manifold-hardwarnings OPENSCAD SUFFIX png FILES ${MANIFOLDHARDWARNING_FILES} EXPECTEDDIR render ARGS --render=force --backend=manifold --hardwarnings)
add_cmdline_test(preview-manifold OPENSCAD SUFFIX png FILES ${PREVIEW_COMMON_FILES} EXPECTEDDIR preview ARGS --backend=manifold)
add_cmdline_test(preview-manifold OPENSCAD SUFFIX png FILES ${PREVIEW_DIFFERENT_EXPECTATIONS} ARGS --backend=manifold)
endif()

add_cmdline_test(throwntogether-manifold  OPENSCAD FILES ${ALL_THROWNTOGETHER_FILES} SUFFIX png EXPECTEDDIR throwntogether ARGS --preview=throwntogether --backend=manifold)
