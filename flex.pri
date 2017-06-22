{
  flex.name = Flex ${QMAKE_FILE_IN}
  flex.input = FLEXSOURCES
  flex.output = $${OBJECTS_DIR}/${QMAKE_FILE_BASE}.cxx
  flex.commands = flex -o $${OBJECTS_DIR}/${QMAKE_FILE_BASE}.cxx --header-file=$${OBJECTS_DIR}/${QMAKE_FILE_BASE}.hxx ${QMAKE_FILE_IN}
  flex.CONFIG += target_predeps
  flex.variable_out = GENERATED_SOURCES
  silent:flex.commands = @echo Lex ${QMAKE_FILE_IN} && $$flex.commands
  QMAKE_EXTRA_COMPILERS += flex
}
