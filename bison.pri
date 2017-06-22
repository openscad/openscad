{
  bison.name = Bison ${QMAKE_FILE_IN}
  bison.input = BISONSOURCES
  bison.output = $${OBJECTS_DIR}/${QMAKE_FILE_BASE}.cxx
  bison.commands = bison -d -p ${QMAKE_FILE_BASE} -o $${OBJECTS_DIR}/${QMAKE_FILE_BASE}.cxx --defines=$${OBJECTS_DIR}/${QMAKE_FILE_BASE}.hxx ${QMAKE_FILE_IN}
  bison.CONFIG += target_predeps
  bison.variable_out = GENERATED_SOURCES
  silent:bison.commands = @echo Bison ${QMAKE_FILE_IN} && $$bison.commands
  QMAKE_EXTRA_COMPILERS += bison
}
