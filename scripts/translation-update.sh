#!/bin/sh

# see doc/translation.txt for more info

updatepot()
{
 if [ ! -e objects/ui_MainWindow.h ]; then
   echo cannot find objects/ui_xxxxx.h files. perhaps if you run make...?
   exit 1
 fi
 VER=`date +"%Y.%m.%d"`
 OPTS=
 OPTS=$OPTS' --package-name=OpenSCAD'
 OPTS=$OPTS' --package-version='$VER
 OPTS=$OPTS' --default-domain=openscad'
 OPTS=$OPTS' --keyword=_'
 OPTS=$OPTS' --files-from=./po/POTFILES.in'
 cmd="${GETTEXT_PATH}xgettext "$OPTS' -o ./po/openscad.pot'
 echo $cmd
 $cmd
 if [ ! $? = 0 ]; then
  echo error running xgettext
  exit 1
 fi
 sed -e s/"CHARSET"/"UTF-8"/g ./po/openscad.pot > ./po/openscad.pot.new && mv ./po/openscad.pot.new ./po/openscad.pot
}

updatepo()
{
 for LANGCODE in `cat ./po/LINGUAS | grep -v "#"`; do
  OPTS='--update --backup=t'
  cmd="$GETTEXT_PATH"'msgmerge '$OPTS' ./po/'$LANGCODE'.po ./po/openscad.pot'
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
 for LANGCODE in `cat po/LINGUAS | grep -v "#"`; do
  mkdir -p ./po/$LANGCODE/LC_MESSAGES
  OPTS='-c -v'
  cmd="$GETTEXT_PATH"'msgfmt '$OPTS' -o ./po/'$LANGCODE'/LC_MESSAGES/openscad.mo ./po/'$LANGCODE'.po'
  echo $cmd
  $cmd
  if [ ! $? = 0 ]; then
   echo error running msgfmt
   exit 1
  fi
 done
}

GETTEXT_PATH=""
if [ "x$OPENSCAD_LIBRARIES" != x ]; then
	GETTEXT_PATH="$OPENSCAD_LIBRARIES/bin/"
fi

if [ "x$1" = xupdatemo ]; then
 updatemo
else
 updatepot && updatepo && updatemo
fi

