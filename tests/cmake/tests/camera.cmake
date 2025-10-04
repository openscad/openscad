# Test transcription of --camera options to $vp* for 7-argument --camera.
add_cmdline_test(echo-cli-view-variables-7         OPENSCAD SUFFIX echo FILES ${TEST_SCAD_DIR}/misc/test-view-variables.scad ARGS --camera 10,20,30,40,50,60,70)
add_cmdline_test(dump-cli-view-variables-7         OPENSCAD SUFFIX csg FILES ${TEST_SCAD_DIR}/misc/test-view-variables.scad ARGS --camera 10,20,30,40,50,60,70)
add_cmdline_test(preview-cli-view-variables-7      OPENSCAD SUFFIX png FILES ${TEST_SCAD_DIR}/misc/test-view-variables.scad ARGS --camera 10,20,30,40,50,60,70)

# Test transcription of --camera options to $vp* for 6-argument --camera.
add_cmdline_test(echo-cli-view-variables-6         OPENSCAD SUFFIX echo FILES ${TEST_SCAD_DIR}/misc/test-view-variables.scad ARGS --camera 10,20,30,40,50,60)
add_cmdline_test(dump-cli-view-variables-6         OPENSCAD SUFFIX csg FILES ${TEST_SCAD_DIR}/misc/test-view-variables.scad ARGS --camera 10,20,30,40,50,60)
add_cmdline_test(preview-cli-view-variables-6      OPENSCAD SUFFIX png FILES ${TEST_SCAD_DIR}/misc/test-view-variables.scad ARGS --camera 10,20,30,40,50,60)

set(CAMERA_TEST           "${TEST_SCAD_DIR}/3D/misc/camera-tests.scad")
set(CAMERA_TEST_OFFCENTER "${TEST_SCAD_DIR}/3D/misc/camera-tests-offcenter.scad")
set(CAMERA_TEST_VP        "${TEST_SCAD_DIR}/3D/misc/camera-vp.scad")
# Image output parameters
add_cmdline_test(openscad-imgsize          OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS --imgsize 100,100)
add_cmdline_test(openscad-imgstretch       OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS --imgsize 500,100)
add_cmdline_test(openscad-imgstretch2      OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS --imgsize 100,500)
# Perspective gimbal cam
set(IMGSIZE "--imgsize=500,500")
add_cmdline_test(openscad-camdist          OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=0,0,0,90,0,90,200)
add_cmdline_test(openscad-camrot           OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=0,0,0,440,337.5,315,200)
add_cmdline_test(openscad-camtrans         OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=100,-20,-10,90,0,90,200)
add_cmdline_test(openscad-camtrans-viewall OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=100,-20,-10,90,0,90,6000 --viewall)
add_cmdline_test(openscad-camtrans-viewall-offcenter OPENSCAD FILES ${CAMERA_TEST_OFFCENTER} SUFFIX png ARGS ${IMGSIZE} --camera=0,0,0,30,40,50,10 --viewall --autocenter)
# Orthographic gimbal cam
add_cmdline_test(openscad-camortho         OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=100,-20,-20,90,0,90,220 --projection=o)
add_cmdline_test(openscad-camortho-viewall OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=100,-20,-10,90,0,90,3000 --viewall --projection=o)
# Perspective vector cam
add_cmdline_test(openscad-cameye            OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=120,80,60,0,0,0)
add_cmdline_test(openscad-cameye_front      OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=0,-130,0,0,0,0)
add_cmdline_test(openscad-cameye_back       OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=0,130,0,0,0,0)
add_cmdline_test(openscad-cameye_left       OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=-130,0,0,0,0,0)
add_cmdline_test(openscad-cameye_right      OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=130,0,0,0,0,0)
add_cmdline_test(openscad-cameye_top        OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=0,0,130,0,0,0)
add_cmdline_test(openscad-cameye_bottom     OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=0,0,-130,0,0,0)
add_cmdline_test(openscad-cameye2           OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=160,140,130,0,0,0)
add_cmdline_test(openscad-camcenter         OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=100,60,30,20,10,30)
add_cmdline_test(openscad-camcenter-viewall OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=60,40,30,20,10,30 --viewall)
# Orthographic vector cam
add_cmdline_test(openscad-cameyeortho         OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=90,80,75,0,0,0 --projection=o)
add_cmdline_test(openscad-cameyeortho-viewall OPENSCAD FILES ${CAMERA_TEST} SUFFIX png ARGS ${IMGSIZE} --camera=16,14,13,0,0,0 --viewall --projection=o)

add_cmdline_test(openscad-camvp-variables     OPENSCAD FILES ${CAMERA_TEST_VP} SUFFIX png ARGS ${IMGSIZE})
add_cmdline_test(openscad-camvp-override      OPENSCAD FILES ${CAMERA_TEST_VP} SUFFIX png ARGS ${IMGSIZE} --camera=120,80,60,0,0,0)
