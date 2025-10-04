list(APPEND EXPORT_3MF_TEST_FILES ${TEST_SCAD_DIR}/3mf/3mf-export.scad)

if (LIB3MF_FOUND)
add_cmdline_test(export-3mf              EXPERIMENTAL OPENSCAD SUFFIX 3mf FILES ${EXPORT_3MF_TEST_FILES} ARGS --enable=predictible-output)
endif()

set(render-3mf-manifold_FILES ${render-off-manifold_FILES})

if (LIB3MF_FOUND)
add_cmdline_test(preview-3mf         SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_PREVIEW_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=3MF)
add_cmdline_test(render-3mf-cgal     SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDER_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=3MF --render=force --backend=cgal)
add_cmdline_test(render-3mf-cgal     SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_DIFFERENT_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=3MF --render=force --backend=cgal)
add_cmdline_test(render-3mf-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDER_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=3MF --render=force --backend=manifold)
add_cmdline_test(render-3mf-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_DIFFERENT_FILES} EXPECTEDDIR render ARGS ${OPENSCAD_EXE_ARG} --format=3MF --render=force --backend=manifold)
add_cmdline_test(render-3mf-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_DIFFERENT_MANIFOLD_FILES} EXPECTEDDIR render-manifold ARGS ${OPENSCAD_EXE_ARG} --format=3MF --render=force --backend=manifold)
endif()

add_cmdline_test(3mfcolorpngtest EXPERIMENTAL SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${COLOR_3D_TEST_FILES} EXPECTEDDIR render-manifold ARGS ${OPENSCAD_EXE_ARG} --format=3MF --backend=manifold --render)
