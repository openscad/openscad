#!/usr/bin/env bash

set -euo pipefail

# see doc/translation.txt for more info
BASEDIR=$PWD

updatepot()
{
 # check we have all files from POTFILES present
 while read f
 do
   if [ ! -f "$f" ]; then
     echo "cannot find file '$f' from POTFILES"
     exit 1
   fi
 done < locale/POTFILES

 grep ui_MainWindow.h locale/POTFILES >/dev/null 2>/dev/null
 if [ $? -ne 0 ] ; then
   echo "cannot find .../ui_xxxxx.h files. perhaps if you run make...?"
   exit 1
 fi

 # example folder names
 find examples -mindepth 1 -maxdepth 1 -type d -printf "%f\n" \
	| awk '{ printf "#: examples\nmsgid \"%s\"\nmsgstr \"\"\n\n", $1 }' \
	> ./locale/json-strings.pot

 # extract strings from appdata file
 itstool -o ./locale/appdata-strings.pot ./pythonscad.appdata.xml.in2 --its=./contrib/appdata.its

 VER=`date +"%Y.%m.%d"`
 OPTS=
 OPTS=$OPTS' --package-name=OpenSCAD'
 OPTS=$OPTS' --package-version='$VER
 OPTS=$OPTS' --default-domain=openscad'
 OPTS=$OPTS' --language=c++'
 OPTS=$OPTS' --keyword=' #without WORD means not to use default keywords
 OPTS=$OPTS' --keyword=_'
 OPTS=$OPTS' --keyword=q_'
 OPTS=$OPTS' --keyword=_:1,2c'
 OPTS=$OPTS' --keyword=q_:1,2c'
 OPTS=$OPTS' --keyword=ngettext:1,2'
 OPTS=$OPTS' --files-from=./locale/POTFILES'
 cmd="${GETTEXT_PATH}xgettext "$OPTS' -o ./locale/pythonscad-tmp.pot'
 echo $cmd
 $cmd
 if [ ! $? = 0 ]; then
  echo error running xgettext
  exit 1
 fi
 cmd="${GETTEXT_PATH}msgcat -o ./locale/pythonscad.pot ./locale/pythonscad-tmp.pot ./locale/json-strings.pot ./locale/appdata-strings.pot"
 echo $cmd
 $cmd
 if [ ! $? = 0 ]; then
  echo error running msgcat
  exit 1
 fi

 sed -e s/"CHARSET"/"UTF-8"/g ./locale/pythonscad.pot > ./locale/pythonscad.pot.new && mv ./locale/pythonscad.pot.new ./locale/pythonscad.pot
 rm -f ./locale/json-strings.pot ./locale/pythonscad-tmp.pot ./locale/appdata-strings.pot
}

updatepo()
{
 for LANGCODE in `cat ./locale/LINGUAS | grep -v "#"`; do
  OPTS='--update --backup=t'
  cmd="$GETTEXT_PATH"'msgmerge '$OPTS' ./locale/'$LANGCODE'.po ./locale/pythonscad.pot'
  echo $cmd
  $cmd
  if [ ! $? = 0 ]; then
   echo error running msgmerge
   exit 1
  fi
 done
}

updatemo()
{
  for LANGCODE in `cat locale/LINGUAS | grep -v "#"`; do
    mkdir -p ./locale/$LANGCODE/LC_MESSAGES
    OPTS='-c -v'
    cmd="$GETTEXT_PATH"'msgfmt '$OPTS' -o ./locale/'$LANGCODE'/LC_MESSAGES/pythonscad.mo ./locale/'$LANGCODE'.po'
    echo $cmd
    $cmd
    if [ ! $? = 0 ]; then
      echo error running msgfmt
      exit 1
    fi
  done

  if which itstool > /dev/null 2>&1; then
    # ugly workaround for bug https://bugs.freedesktop.org/show_bug.cgi?id=90937
    for LANGCODE in `cat locale/LINGUAS | grep -v "#"`; do
      ln -s pythonscad.mo ./locale/$LANGCODE/LC_MESSAGES/$LANGCODE.mo
    done

    # generate translated appdata file
    itstool -j ./pythonscad.appdata.xml.in2 -o ./pythonscad.appdata.xml ./locale/*/LC_MESSAGES/[a-z][a-z].mo

    # clean the mess
    for LANGCODE in `cat locale/LINGUAS | grep -v "#"`; do
      unlink ./locale/$LANGCODE/LC_MESSAGES/$LANGCODE.mo
    done
  else
    if [ x"$(uname -s)" = x"Linux" ]; then
      echo "itstool missing, won't apply translations to pythonscad.appdata.xml"
    fi
    cp -f ./pythonscad.appdata.xml.in2 ./pythonscad.appdata.xml
  fi
}

GETTEXT_PATH=""
#if [ "x$OPENSCAD_LIBRARIES" != x ]; then
#	GETTEXT_PATH="$OPENSCAD_LIBRARIES/bin/"
#fi

SCRIPTDIR="`dirname \"$0\"`"
TOPDIR="`dirname \"$SCRIPTDIR\"`"
CURDIR="`python3 -c "import os; print(os.path.relpath(os.path.realpath('$BASEDIR'), '$TOPDIR'))"`"

cd "$TOPDIR" || exit 1

if [ -z ${1+x} ]; then
  echo "Generating POTFILES..."
  BUILDDIR=$(
	find "$CURDIR" -name ui_MainWindow.h \
	| grep OpenSCAD.*_autogen/include/ui_MainWindow.h \
	| sed -e 's,/*OpenSCAD.*_autogen/include/ui_MainWindow.h,,'
  )
  echo "Found directory: $BUILDDIR"
  ./scripts/generate-potfiles.sh "$BUILDDIR" > locale/POTFILES
  updatepot && updatepo && updatemo
elif [ "$1" = updatemo ]; then
  updatemo
else
  echo "usage: $0 [updatemo]"
fi
