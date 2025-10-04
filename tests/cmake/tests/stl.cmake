list(APPEND EXPORT_STL_TEST_FILES
  ${TEST_SCAD_DIR}/stl/stl-export.scad
)

add_cmdline_test(export-stl              EXPERIMENTAL OPENSCAD SUFFIX stl FILES ${EXPORT_STL_TEST_FILES} ARGS --enable=predictible-output --render)
add_cmdline_test(export-stl-stdout       EXPERIMENTAL OPENSCAD SUFFIX stl FILES ${EXPORT_STL_TEST_FILES} STDIO EXPECTEDDIR export-stl ARGS --enable=predictible-output --render --export-format asciistl)

if (ENABLE_MANIFOLD)
add_cmdline_test(export-stl-manifold     EXPERIMENTAL OPENSCAD SUFFIX stl FILES ${EXPORT_STL_TEST_FILES} EXPECTEDDIR export-stl ARGS --enable=predictible-output --backend=manifold --render)
endif()

add_cmdline_test(export-binstl           EXPERIMENTAL OPENSCAD SUFFIX stl FILES ${EXPORT_STL_TEST_FILES} ARGS --enable=predictible-output --render --export-format binstl)
add_cmdline_test(export-binstl-stdout    EXPERIMENTAL OPENSCAD SUFFIX stl FILES ${EXPORT_STL_TEST_FILES} STDIO EXPECTEDDIR export-binstl ARGS --enable=predictible-output --render --export-format binstl)

add_cmdline_test(preview-stl SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_PREVIEW_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=STL)

add_cmdline_test(render-stl SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDER_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=STL --render=force)

if (ENABLE_MANIFOLD)
add_cmdline_test(render-stl-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${EXPORT_IMPORT_3D_RENDERMANIFOLD_FILES} EXPECTEDDIR render-monotone ARGS ${OPENSCAD_EXE_ARG} --format=STL --render=force --backend=manifold)
add_cmdline_test(render-stl-manifold SCRIPT ${EXPORT_IMPORT_PNGTEST_PY} SUFFIX png FILES ${FILES_MANIFOLD_CORNER_CASES} EXPECTEDDIR render-off-manifold ARGS ${OPENSCAD_EXE_ARG} --format=STL --render=force --backend=manifold)
endif(ENABLE_MANIFOLD)

add_cmdline_test(export-stl-sanitytest  SCRIPT ${STLEXPORTSANITYTEST_PY} SUFFIX txt FILES ${TEST_SCAD_DIR}/misc/normal-nan.scad ARGS ${OPENSCAD_EXE_ARG})
