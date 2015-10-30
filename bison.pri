{
  bison.name = Bison ${QMAKE_FILE_IN}
  bison.input = BISONSOURCES
  bison.output = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}_yacc.cpp
  bison.commands = bison -d -p ${QMAKE_FILE_BASE} -o ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}_yacc.cpp ${QMAKE_FILE_IN}
  bison.commands += && if [[ -e ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}_yacc.hpp ]] ; then mv ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}_yacc.hpp ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}_yacc.h ; fi
  bison.CONFIG += target_predeps
  bison.variable_out = GENERATED_SOURCES
  silent:bison.commands = @echo Bison ${QMAKE_FILE_IN} && $$bison.commands
  QMAKE_EXTRA_COMPILERS += bison
  bison_header.input = BISONSOURCES
  bison_header.output = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}_yacc.h
  bison_header.commands = sleep 2 && bison -d -p ${QMAKE_FILE_BASE} -o ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}_yacc.cpp ${QMAKE_FILE_IN}
  bison_header.commands += && if [ -e ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}_yacc.hpp ]; then mv ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}_yacc.hpp ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}_yacc.h ; fi
  bison_header.CONFIG += target_predeps no_link
  silent:bison_header.commands = @echo Bison ${QMAKE_FILE_IN} && $$bison.commands
  QMAKE_EXTRA_COMPILERS += bison_header
}

unix:linux* {
  exists(/usr/bin/bison) {
    QMAKE_YACC = /usr/bin/bison
  }
}

freebsd* {
  # on some BSD, /usr/local/bin/bison is newer than
  # /usr/bin/bison, so try to prefer it.
  exists(/usr/local/bin/bison) {
    QMAKE_YACC = /usr/local/bin/bison
  } else { # look in $PATH
    QMAKE_YACC = bison
  }
}

netbsd* {
  exists(/usr/pkg/bin/bison) {
    QMAKE_YACC = /usr/pkg/bin/bison
  } else { # look in $PATH
    QMAKE_YACC = bison
  }
}

win32*msvc* {
}
