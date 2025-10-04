############################
# Experimental tests       #
############################

list(APPEND LAZYUNION_3D_FILES
  ${TEST_SCAD_DIR}/experimental/lazyunion-toplevel-objects.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-toplevel-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-nested-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-children.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-hull-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-root-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-intersection-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-difference-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-minkowski-for.scad
  ${TEST_SCAD_DIR}/experimental/lazyunion-transform-for.scad
  ${TEST_SCAD_DIR}/3D/features/2d-3d.scad
)
list(APPEND LAZYUNION_2D_FILES
  ${TEST_SCAD_DIR}/experimental/lazyunion-toplevel-2dobjects.scad
)
list(APPEND LAZYUNION_FILES ${LAZYUNION_2D_FILES} ${LAZYUNION_3D_FILES})

#
# --enable=lazyunion tests
#
add_cmdline_test(lazyunion-export-csg  EXPERIMENTAL OPENSCAD SUFFIX csg FILES ${LAZYUNION_FILES} ARGS --enable=lazy-union)
add_cmdline_test(lazyunion-preview     EXPERIMENTAL OPENSCAD SUFFIX png FILES ${LAZYUNION_FILES} ARGS --enable=lazy-union)
add_cmdline_test(lazyunion-render      EXPERIMENTAL OPENSCAD SUFFIX png FILES ${LAZYUNION_FILES} ARGS --enable=lazy-union --render)
add_cmdline_test(lazyunion-render-stl  EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${LAZYUNION_3D_FILES} ARGS ${OPENSCAD_EXE_ARG} --format=STL --enable=lazy-union --render=force)
add_cmdline_test(lazyunion-render-dxf  EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${LAZYUNION_2D_FILES} EXPECTEDDIR lazyunion-render ARGS ${OPENSCAD_EXE_ARG} --format=DXF --enable=lazy-union --render=force)
add_cmdline_test(lazyunion-render-svg  EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${LAZYUNION_2D_FILES} EXPECTEDDIR lazyunion-render ARGS ${OPENSCAD_EXE_ARG} --format=SVG --enable=lazy-union --render=force)

#
# --enable=roof tests
#
list(APPEND EXPERIMENTAL_ROOF_FILES ${EXAMPLES_DIR}/Basics/roof.scad)
add_cmdline_test(dump-examples    EXPERIMENTAL OPENSCAD SUFFIX csg FILES ${EXPERIMENTAL_ROOF_FILES} ARGS --enable=roof)
add_cmdline_test(render-cgal      EXPERIMENTAL OPENSCAD SUFFIX png FILES ${EXPERIMENTAL_ROOF_FILES} ARGS --render --enable=roof --backend=cgal)
add_cmdline_test(render-manifold  EXPERIMENTAL OPENSCAD SUFFIX png FILES ${EXPERIMENTAL_ROOF_FILES} EXPECTEDDIR render ARGS --render --enable=roof --backend=manifold)
add_cmdline_test(preview-cgal     EXPERIMENTAL OPENSCAD SUFFIX png FILES ${EXPERIMENTAL_ROOF_FILES} EXPECTEDDIR preview ARGS --enable=roof --backend=cgal)
add_cmdline_test(preview-manifold EXPERIMENTAL OPENSCAD SUFFIX png FILES ${EXPERIMENTAL_ROOF_FILES} ARGS --enable=roof --backend=manifold)
add_cmdline_test(throwntogether   EXPERIMENTAL OPENSCAD SUFFIX png FILES ${EXPERIMENTAL_ROOF_FILES} ARGS --preview=throwntogether --enable=roof)
add_cmdline_test(render-csg-cgal        EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPERIMENTAL_ROOF_FILES} EXPECTEDDIR render ARGS ${OPENSCAD_EXE_ARG} --format=csg --render --enable=roof)

#
# --enable=import-function tests
#
list(APPEND EXPERIMENTAL_IMPORT_FILES
  ${TEST_SCAD_DIR}/json/import-json.scad
  ${TEST_SCAD_DIR}/json/import-json-relative-path.scad
  )
add_cmdline_test(echo           EXPERIMENTAL OPENSCAD SUFFIX echo FILES ${EXPERIMENTAL_IMPORT_FILES} ARGS --enable=import-function)

#
# --enable=textmetrics tests
#
list(APPEND EXPERIMENTAL_TEXTMETRICS_ECHO_FILES
  ${TEST_SCAD_DIR}/misc/isobject-test.scad
  ${TEST_SCAD_DIR}/misc/text-metrics-test.scad
)
list(APPEND EXPERIMENTAL_TEXTMETRICS_FILES
  ${TEST_SCAD_DIR}/2D/features/text-metrics.scad
)
add_cmdline_test(echo           EXPERIMENTAL OPENSCAD SUFFIX echo FILES ${EXPERIMENTAL_TEXTMETRICS_ECHO_FILES} ARGS --enable=textmetrics)
add_cmdline_test(dump           EXPERIMENTAL OPENSCAD SUFFIX csg  FILES ${EXPERIMENTAL_TEXTMETRICS_FILES} ARGS --enable=textmetrics)
add_cmdline_test(preview-cgal   EXPERIMENTAL OPENSCAD SUFFIX png  FILES ${EXPERIMENTAL_TEXTMETRICS_FILES} EXPECTEDDIR preview ARGS --enable=textmetrics --backend=cgal)
add_cmdline_test(throwntogether EXPERIMENTAL OPENSCAD SUFFIX png  FILES ${EXPERIMENTAL_TEXTMETRICS_FILES} ARGS --preview=throwntogether --enable=textmetrics)
add_cmdline_test(render-csg-cgal      EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPERIMENTAL_TEXTMETRICS_FILES} EXPECTEDDIR render ARGS ${OPENSCAD_EXE_ARG} --format=csg --render --enable=textmetrics)
add_cmdline_test(render-cgal    EXPERIMENTAL OPENSCAD SUFFIX png FILES ${EXPERIMENTAL_TEXTMETRICS_FILES} EXPECTEDDIR render ARGS --render --enable=textmetrics --backend=cgal)
add_cmdline_test(render-dxf      EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png ARGS ${OPENSCAD_EXE_ARG} --format=DXF --render=force --enable=textmetrics EXPECTEDDIR render FILES ${EXPERIMENTAL_TEXTMETRICS_FILES})
add_cmdline_test(render-svg      EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png ARGS ${OPENSCAD_EXE_ARG} --format=SVG --render=force --enable=textmetrics EXPECTEDDIR render FILES ${EXPERIMENTAL_TEXTMETRICS_FILES})
