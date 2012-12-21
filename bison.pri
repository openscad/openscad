win32 {
  bison.name = Bison ${QMAKE_FILE_IN}
  bison.input = BISONSOURCES
  bison.output = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}_yacc.cpp
  bison.commands = bison -d -p ${QMAKE_FILE_BASE} -o ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}_yacc.cpp ${QMAKE_FILE_IN}
  bison.commands += && mv ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}_yacc.hpp ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}_yacc.h
  bison.CONFIG += target_predeps
  bison.variable_out = GENERATED_SOURCES
  silent:bison.commands = @echo Bison ${QMAKE_FILE_IN} && $$bison.commands
  QMAKE_EXTRA_COMPILERS += bison
  bison_header.input = BISONSOURCES
  bison_header.output = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}_yacc.h
  bison_header.commands = bison -d -p ${QMAKE_FILE_BASE} -o ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}_yacc.cpp ${QMAKE_FILE_IN}
  bison_header.commands += && mv ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}_yacc.hpp ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}_yacc.h
  bison_header.CONFIG += target_predeps no_link
  silent:bison_header.commands = @echo Bison ${QMAKE_FILE_IN} && $$bison.commands
  QMAKE_EXTRA_COMPILERS += bison_header
}

unix:freebsd-g++ {
  # on bsd /usr/bin/bison is outdated, dont use it
  exists(/usr/local/bin/bison) {
    QMAKE_YACC = /usr/local/bin/bison
  } else { # look in $PATH
    QMAKE_YACC = bison
  }
}

unix:netbsd* {
  exists(/usr/pkg/bin/bison) {
    QMAKE_YACC = /usr/pkg/bin/bison
  } else { # look in $PATH
    QMAKE_YACC = bison
  }
}

unix:linux* {
  exists(/usr/bin/bison) {
    QMAKE_YACC = /usr/bin/bison
  }
}
