# FIXME(kintel): Not in use anymore, but keep for reference?
list(APPEND SCADFILES_WITH_GREEN_FACES
  ${EXAMPLES_DIR}/Advanced/children.scad
  ${EXAMPLES_DIR}/Basics/CSG-modules.scad
  ${EXAMPLES_DIR}/Basics/CSG.scad
  ${EXAMPLES_DIR}/Basics/logo.scad
  ${EXAMPLES_DIR}/Basics/LetterBlock.scad
  ${EXAMPLES_DIR}/Basics/text_on_cube.scad
  ${EXAMPLES_DIR}/Basics/logo_and_text.scad
  ${EXAMPLES_DIR}/Parametric/sign.scad
  ${EXAMPLES_DIR}/Parametric/candleStand.scad
  ${EXAMPLES_DIR}/Old/example001.scad
  ${EXAMPLES_DIR}/Old/example002.scad
  ${EXAMPLES_DIR}/Old/example003.scad
  ${EXAMPLES_DIR}/Old/example004.scad
  ${EXAMPLES_DIR}/Old/example005.scad
  ${EXAMPLES_DIR}/Old/example006.scad
  ${EXAMPLES_DIR}/Old/example007.scad
  ${EXAMPLES_DIR}/Old/example012.scad
  ${EXAMPLES_DIR}/Old/example016.scad
  ${EXAMPLES_DIR}/Old/example024.scad
  ${TEST_SCAD_DIR}/3D/features/difference-tests.scad
  ${TEST_SCAD_DIR}/3D/features/for-tests.scad
  ${TEST_SCAD_DIR}/3D/features/highlight-modifier2.scad
  ${TEST_SCAD_DIR}/3D/features/minkowski3-erosion.scad
  ${TEST_SCAD_DIR}/3D/features/minkowski3-difference-test.scad
  ${TEST_SCAD_DIR}/3D/features/render-tests.scad
  ${TEST_SCAD_DIR}/3D/features/resize-convexity-tests.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105b.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105c.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1105d.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1215c.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1803.scad
  ${TEST_SCAD_DIR}/3D/issues/issue3158.scad
  ${TEST_SCAD_DIR}/3D/issues/issue835.scad
  ${TEST_SCAD_DIR}/3D/issues/issue911.scad
  ${TEST_SCAD_DIR}/3D/issues/issue913.scad
  ${TEST_SCAD_DIR}/3D/issues/issue904.scad
  ${TEST_SCAD_DIR}/3D/issues/issue1165.scad
  ${TEST_SCAD_DIR}/3D/misc/view-options-tests.scad
  ${TEST_SCAD_DIR}/misc/include-tests.scad
  ${TEST_SCAD_DIR}/misc/use-tests.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity-polyhedron.scad
  ${TEST_SCAD_DIR}/misc/internal-cavity.scad
  ${TEST_SCAD_DIR}/misc/let-module-tests.scad
  ${TEST_SCAD_DIR}/misc/rotate_extrude-hole.scad
  ${TEST_SCAD_DIR}/misc/rotate-empty-bbox.scad
)

list(APPEND NEF3_BROKEN_TESTS
  render-cgal_nef3_broken
  preview-cgal_nef3_broken
  throwntogether-cgal_nef3_broken
  render-csg-cgal_nef3_broken
)

list(APPEND DUMP_FILES ${FEATURES_2D_FILES} ${FEATURES_3D_FILES} ${DEPRECATED_3D_FILES} ${MISC_FILES})

list(APPEND ASTDUMP_FILES ${MISC_FILES}
  ${TEST_SCAD_DIR}/functions/assert-expression-fail1-test.scad
  ${TEST_SCAD_DIR}/functions/assert-expression-fail2-test.scad
  ${TEST_SCAD_DIR}/functions/assert-expression-fail3-test.scad
  ${TEST_SCAD_DIR}/functions/assert-expression-tests.scad
  ${TEST_SCAD_DIR}/functions/echo-expression-tests.scad
  ${TEST_SCAD_DIR}/functions/expression-precedence-tests.scad
  ${TEST_SCAD_DIR}/functions/let-test-single.scad
  ${TEST_SCAD_DIR}/functions/let-tests.scad
  ${TEST_SCAD_DIR}/functions/list-comprehensions.scad
  ${TEST_SCAD_DIR}/functions/exponent-operator-test.scad
  ${TEST_SCAD_DIR}/misc/ifelse-ast-dump.scad
  ${TEST_SCAD_DIR}/svg/id-layer-selection-test.scad
)

#FIXME: Reintroduce
list(APPEND COLOR_EXPORT_TEST_FILES
  ${TEST_SCAD_DIR}/misc/color-export.scad
)
