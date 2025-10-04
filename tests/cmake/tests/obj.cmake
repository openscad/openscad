list(APPEND EXPORT_OBJ_TEST_FILES ${TEST_SCAD_DIR}/obj/obj-export.scad)
list(APPEND EXPORT_OBJ_TEST_FILES ${TEST_SCAD_DIR}/obj/obj-import-export_dodecahedron.scad)
list(APPEND EXPORT_OBJ_TEST_FILES ${TEST_SCAD_DIR}/obj/obj-import-export_cube.scad)
list(APPEND EXPORT_OBJ_TEST_FILES ${TEST_SCAD_DIR}/3D/features/polyhedron-cube.scad)

add_cmdline_test(export-obj              EXPERIMENTAL OPENSCAD SUFFIX obj FILES ${EXPORT_OBJ_TEST_FILES} ARGS --enable=predictible-output)

add_cmdline_test(preview-obj SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_PREVIEW_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=OBJ)

add_cmdline_test(render-obj SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDER_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=OBJ --render=force)

if (ENABLE_MANIFOLD)
add_cmdline_test(render-obj-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDERMANIFOLD_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=OBJ --render=force --backend=manifold)
add_cmdline_test(render-obj-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${FILES_MANIFOLD_CORNER_CASES} EXPECTEDDIR render-off-manifold ARGS ${OPENSCAD_EXE_ARG} --format=OBJ --render=force --backend=manifold)
endif(ENABLE_MANIFOLD)
