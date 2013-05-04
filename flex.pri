{
  flex.name = Flex ${QMAKE_FILE_IN}
  flex.input = FLEXSOURCES
  flex.output = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.lexer.cpp
  flex.commands = flex -P ${QMAKE_FILE_BASE} -o${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.lexer.cpp ${QMAKE_FILE_IN}
  flex.CONFIG += target_predeps
  flex.variable_out = GENERATED_SOURCES
  silent:flex.commands = @echo Lex ${QMAKE_FILE_IN} && $$flex.commands
  QMAKE_EXTRA_COMPILERS += flex
}

unix:linux* {
  exists(/usr/bin/flex) {
    QMAKE_LEX = /usr/bin/flex
  }
}

freebsd* {
  QMAKE_LEX = /usr/local/bin/flex
}

netbsd* {
  QMAKE_LEX = /usr/pkg/bin/flex
}

