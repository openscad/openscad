#!/bin/sh

# see doc/translation.txt for more info

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

 VER=`date +"%Y.%m.%d"`
 OPTS=
 OPTS=$OPTS' --package-name=OpenSCAD'
 OPTS=$OPTS' --package-version='$VER
 OPTS=$OPTS' --default-domain=openscad'
 OPTS=$OPTS' --keyword=_'
 OPTS=$OPTS' --keyword=N_'
 OPTS=$OPTS' --files-from=./locale/POTFILES'
 cmd="${GETTEXT_PATH}xgettext "$OPTS' -o ./locale/openscad.pot'
 echo $cmd
 $cmd
 if [ ! $? = 0 ]; then
  echo error running xgettext
  exit 1
 fi
 sed -e s/"CHARSET"/"UTF-8"/g ./locale/openscad.pot > ./locale/openscad.pot.new && mv ./locale/openscad.pot.new ./locale/openscad.pot
}

updatepo()
{
 for LANGCODE in `cat ./locale/LINGUAS | grep -v "#"`; do
  OPTS='--update --backup=t'
  cmd="$GETTEXT_PATH"'msgmerge '$OPTS' ./locale/'$LANGCODE'.po ./locale/openscad.pot'
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
  cmd="$GETTEXT_PATH"'msgfmt '$OPTS' -o ./locale/'$LANGCODE'/LC_MESSAGES/openscad.mo ./locale/'$LANGCODE'.po'
  echo $cmd
  $cmd
  if [ ! $? = 0 ]; then
   echo error running msgfmt
   exit 1
  fi
 done
}

GETTEXT_PATH=""
#if [ "x$OPENSCAD_LIBRARIES" != x ]; then
#	GETTEXT_PATH="$OPENSCAD_LIBRARIES/bin/"
#fi

SCRIPTDIR="`dirname \"$0\"`"
TOPDIR="`dirname \"$SCRIPTDIR\"`"

cd "$TOPDIR" || exit 1

if [ "x$1" = xupdatemo ]; then
 updatemo
else
 echo "Generating POTFILES..."
 ./scripts/generate-potfiles.sh > locale/POTFILES
 updatepot && updatepo && updatemo
fi

