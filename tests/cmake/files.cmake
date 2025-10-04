##################################
# Define Various Test File Lists #
##################################

# Find all scad files
file(GLOB FEATURES_2D_FILES   ${TEST_SCAD_DIR}/2D/features/*.scad)
list(REMOVE_ITEM FEATURES_2D_FILES
  ${TEST_SCAD_DIR}/2D/features/text-metrics.scad # -> EXPERIMENTAL_TEXTMETRICS_FILES
)
file(GLOB ISSUES_2D_FILES     ${TEST_SCAD_DIR}/2D/issues/*.scad)
file(GLOB_RECURSE BUGS_2D_FILES    ${TEST_SCAD_DIR}/bugs2D/*.scad)
file(GLOB SCAD_DXF_FILES      ${TEST_SCAD_DIR}/dxf/*.scad)
file(GLOB SCAD_PDF_FILES      ${TEST_SCAD_DIR}/pdf/*.scad)
file(GLOB SCAD_SVG_FILES      ${TEST_SCAD_DIR}/svg/svg-spec/*.scad
  ${TEST_SCAD_DIR}/svg/box-w-holes-2d.scad
  ${TEST_SCAD_DIR}/svg/display.scad
  ${TEST_SCAD_DIR}/svg/id-selection-test.scad
  ${TEST_SCAD_DIR}/svg/id-layer-selection-test.scad
  ${TEST_SCAD_DIR}/svg/line-cap-line-join.scad
  ${TEST_SCAD_DIR}/svg/simple-center-2d.scad
  ${TEST_SCAD_DIR}/svg/use-transform.scad
  ${TEST_SCAD_DIR}/svg/fill-rule.scad
  ${TEST_SCAD_DIR}/svg/size-percent.scad)
  list(APPEND EXAMPLE_2D_FILES
  ${EXAMPLES_DIR}/Old/example015.scad
  ${EXAMPLES_DIR}/Advanced/module_recursion.scad
  ${EXAMPLES_DIR}/Functions/list_comprehensions.scad
  ${EXAMPLES_DIR}/Functions/polygon_areas.scad
  ${EXAMPLES_DIR}/Functions/recursion.scad
)

# used by is2d function
# 2D tests
list(APPEND FILES_2D ${FEATURES_2D_FILES} ${ISSUES_2D_FILES} ${EXAMPLE_2D_FILES})
list(APPEND ALL_2D_FILES
  ${FILES_2D}
  ${SCAD_DXF_FILES}
  ${SCAD_SVG_FILES}
  ${TEST_SCAD_DIR}/2D/features/text-metrics.scad
)

file(GLOB FEATURES_3D_FILES   ${TEST_SCAD_DIR}/3D/features/*.scad)
file(GLOB DEPRECATED_3D_FILES ${TEST_SCAD_DIR}/3D/deprecated/*.scad)
file(GLOB ISSUES_3D_FILES     ${TEST_SCAD_DIR}/3D/issues/*.scad)
file(GLOB SCAD_AMF_FILES           ${TEST_SCAD_DIR}/amf/*.scad)
file(GLOB SCAD_NEF3_FILES          ${TEST_SCAD_DIR}/nef3/*.scad)
file(GLOB_RECURSE BUGS_FILES       ${TEST_SCAD_DIR}/bugs/*.scad)
file(GLOB_RECURSE EXAMPLE_FILES    ${EXAMPLES_DIR}/*.scad)
list(REMOVE_ITEM EXAMPLE_FILES
  ${EXAMPLES_DIR}/Basics/roof.scad # -> EXPERIMENTAL_ROOF_FILES
)

list(APPEND COLOR_3D_TEST_FILES
  ${TEST_SCAD_DIR}/misc/color-cubes.scad
  ${TEST_SCAD_DIR}/3D/features/color-tests3.scad
  ${TEST_SCAD_DIR}/3D/features/linear_extrude-parameter-tests.scad
  ${TEST_SCAD_DIR}/3D/features/resize-tests.scad
)

list(APPEND EXPORT_IMPORT_3D_FILES
  ${TEST_SCAD_DIR}/3D/features/mirror-tests.scad
  ${TEST_SCAD_DIR}/3D/features/polyhedron-nonplanar-tests.scad
  ${TEST_SCAD_DIR}/3D/features/rotate_extrude-tests.scad
  ${TEST_SCAD_DIR}/3D/features/union-coincident-test.scad
  ${TEST_SCAD_DIR}/3D/issues/fn_bug.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105c.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1215.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1215b.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1221.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1225.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1258.scad
  ${TEST_SCAD_DIR}/3D/issues/issue2259.scad
  ${TEST_SCAD_DIR}/3D/issues/issue5216.scad
  ${TEST_SCAD_DIR}/misc/bad-stl-pcbvicebar.scad
  ${TEST_SCAD_DIR}/misc/bad-stl-tardis.scad
  ${TEST_SCAD_DIR}/misc/bad-stl-wing.scad
  ${TEST_SCAD_DIR}/misc/nonmanifold-polyhedron.scad
  ${TEST_SCAD_DIR}/misc/preview_variable.scad
)

list(APPEND EXPORT_IMPORT_3D_DIFFERENT_FILES
  ${TEST_SCAD_DIR}/3D/issues/issue904.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105b.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105d.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1215c.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity-polyhedron.scad
  ${TEST_SCAD_DIR}/misc/rotate_extrude-hole.scad
)

list(APPEND EXPORT_IMPORT_3D_DIFFERENT_MANIFOLD_FILES
  ${TEST_SCAD_DIR}/3D/features/polyhedron-tests.scad
)

# Trivial Export/Import files: Sanity-checks bidirectional file format import/export
set(SIMPLE_EXPORT_IMPORT_2D_FILES ${TEST_SCAD_DIR}/misc/square10.scad)
set(SIMPLE_EXPORT_IMPORT_3D_FILES ${TEST_SCAD_DIR}/misc/cube10.scad)
set(SIMPLE_EXPORT_IMPORT_NON_MANIFOLD ${TEST_SCAD_DIR}/3D/misc/polyhedron-single-triangle.scad)

set(EXPORT_IMPORT_2D_RENDER_FILES ${SIMPLE_EXPORT_IMPORT_2D_TESTS} ${FILES_2D})

set(EXPORT_IMPORT_3D_PREVIEW_FILES ${SIMPLE_EXPORT_IMPORT_NON_MANIFOLD} ${SIMPLE_EXPORT_IMPORT_3D_FILES})
set(EXPORT_IMPORT_3D_RENDER_FILES ${SIMPLE_EXPORT_IMPORT_3D_FILES} ${EXPORT_IMPORT_3D_FILES})
list(REMOVE_ITEM EXPORT_IMPORT_3D_RENDER_FILES
  # Non-manifold polyhedrons can never be rendered
  ${TEST_SCAD_DIR}/misc/nonmanifold-polyhedron.scad
)
set(EXPORT_IMPORT_3D_RENDERMANIFOLD_FILES ${EXPORT_IMPORT_3D_RENDER_FILES})

# Manifoldness corner cases
set(FILES_MANIFOLD_CORNER_CASES
  ${TEST_SCAD_DIR}/3D/misc/tetracyl-slim.scad
  ${TEST_SCAD_DIR}/3D/misc/tetracyl-touch-edge.scad
  ${TEST_SCAD_DIR}/3D/misc/tetracyl-touch-edge-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/tetracyl-touch-vertex.scad
  ${TEST_SCAD_DIR}/3D/misc/tetracyl-touch-vertex-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedrons-touch-edge-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedrons-touch-edge.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedrons-touch-vertex-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedrons-touch-vertex.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedrons-touch-face-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedrons-touch-face.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedron-self-touch-edge-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedron-self-touch-edge.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedron-self-touch-face-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedron-self-touch-face.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedron-self-touch-vertex-nonmanifold.scad
  ${TEST_SCAD_DIR}/3D/misc/polyhedron-self-touch-vertex.scad
  ${TEST_SCAD_DIR}/3D/features/rotate_extrude-touch-edge.scad
  ${TEST_SCAD_DIR}/3D/features/rotate_extrude-touch-vertex.scad
)
