add_cmdline_test(preview-off SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_PREVIEW_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=OFF)
add_cmdline_test(render-off-cgal SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDER_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=OFF --render=force --backend=cgal)
add_cmdline_test(render-off-cgal SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_DIFFERENT_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=OFF --render=force --backend=cgal)

if (ENABLE_MANIFOLD)
set(render-off-manifold_FILES ${EXPORT_IMPORT_3D_RENDERMANIFOLD_FILES})
list(REMOVE_ITEM render-off-manifold_FILES
 ${TEST_SCAD_DIR}/misc/cube10.scad
 ${TEST_SCAD_DIR}/3D/features/polyhedron-tests.scad
)
add_cmdline_test(render-off-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDER_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=OFF --render=force --backend=manifold)
add_cmdline_test(render-off-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_DIFFERENT_FILES} EXPECTEDDIR render ARGS ${OPENSCAD_EXE_ARG} --format=OFF --render=force --backend=manifold)
add_cmdline_test(render-off-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_DIFFERENT_MANIFOLD_FILES} EXPECTEDDIR render-manifold ARGS ${OPENSCAD_EXE_ARG} --format=OFF --render=force --backend=manifold)
add_cmdline_test(render-off-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${FILES_MANIFOLD_CORNER_CASES} ARGS ${OPENSCAD_EXE_ARG} --format=OFF --render=force --backend=manifold)
endif(ENABLE_MANIFOLD)

add_cmdline_test(offcolorpngtest EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${COLOR_3D_TEST_FILES} EXPECTEDDIR render-manifold ARGS ${OPENSCAD_EXE_ARG} --format=OFF --backend=manifold --render)
