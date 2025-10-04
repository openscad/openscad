set(SET_OF_PARAM_TEST "${TEST_CUSTOMIZER_DIR}/setofparameter.scad")
set(SET_OF_PARAM_JSON "${TEST_CUSTOMIZER_DIR}/setofparameter.json")
add_cmdline_test(customizer OPENSCAD ARGS SUFFIX ast FILES
  ${TEST_CUSTOMIZER_DIR}/description.scad
  ${TEST_CUSTOMIZER_DIR}/parameter.scad
  ${TEST_CUSTOMIZER_DIR}/allmodulescomment.scad
  ${TEST_CUSTOMIZER_DIR}/allfunctionscomment.scad
  ${TEST_CUSTOMIZER_DIR}/allexpressionscomment.scad
  ${TEST_CUSTOMIZER_DIR}/group.scad
)
add_cmdline_test(customizer-first          OPENSCAD FILES ${SET_OF_PARAM_TEST} SUFFIX ast ARGS -p ${SET_OF_PARAM_JSON} -P firstSet)
add_cmdline_test(customizer-wrong          OPENSCAD FILES ${SET_OF_PARAM_TEST} SUFFIX ast ARGS -p ${SET_OF_PARAM_JSON} -P wrongSetValues)
add_cmdline_test(customizer-incomplete     OPENSCAD FILES ${SET_OF_PARAM_TEST} SUFFIX ast ARGS -p ${SET_OF_PARAM_JSON} -P thirdSet)
add_cmdline_test(customizer-imgset         OPENSCAD FILES ${SET_OF_PARAM_TEST} SUFFIX ast ARGS -p ${SET_OF_PARAM_JSON} -P imagine)
add_cmdline_test(customizer-setNameWithDot OPENSCAD FILES ${SET_OF_PARAM_TEST} SUFFIX ast ARGS -p ${SET_OF_PARAM_JSON} -P Name.dot)

# Variable override (-D arg)
add_cmdline_test(openscad-override         OPENSCAD FILES ${TEST_SCAD_DIR}/misc/override.scad SUFFIX echo ARGS -D a=3$<SEMICOLON>)
