set(VIEW_OPTIONS_TEST "${TEST_SCAD_DIR}/3D/misc/view-options-tests.scad")
add_cmdline_test(preview-view-axes              OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view axes)
add_cmdline_test(preview-view-axes-scales       OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view axes,scales)
add_cmdline_test(preview-view-axes-scales-edges OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view axes,scales,edges)
add_cmdline_test(render-view-crosshairs         OPENSCAD FILES ${VIEW_OPTIONS_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --render --view crosshairs)
set(VIEW_OPTIONS_EDGES_TEST
  ${VIEW_OPTIONS_TEST}
  ${TEST_SCAD_DIR}/misc/cube10.scad
  ${TEST_SCAD_DIR}/3D/features/render-preserve-colors.scad
)
add_cmdline_test(preview-view-edges-cgal           OPENSCAD FILES ${VIEW_OPTIONS_EDGES_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view edges --backend=cgal)
add_cmdline_test(preview-view-edges-manifold       OPENSCAD FILES ${VIEW_OPTIONS_EDGES_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view edges --backend=manifold)
add_cmdline_test(render-view-edges-manifold OPENSCAD FILES ${VIEW_OPTIONS_EDGES_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=80,14,19,0,0,0 --viewall --view edges --render --backend=manifold)
