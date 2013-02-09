#!/bin/sh

# see doc/translation.txt for more info

help()
{
 echo 'translation.sh [updateall|updatepot|updatepo|updatemo]'
 echo ' updateall - run updatepot, updatepo, and updatemo in that order'
 echo ' updatepot - xgettext: update ./po/openscad.pot from source code files'
 echo ' updatepo - msgmerge: update old .po files with strings in openscad.pot'
 echo ' updatemo - msgfmt: create po/xx/LC_MESSAGES/openscad.mo files'
}

updatepot()
{
 OPTS=
 OPTS=$OPTS'--package-name=OpenSCAD'
 VER=`date +"%Y.%m.%d"`
 OPTS=$OPTS' --package-version='$VER
 OPTS=$OPTS' --default-domain=openscad'
 OPTS=$OPTS' --keyword=_'
 cmd='xgettext '$OPTS' --files-from=./po/POTFILES.in -o ./po/openscad.pot'
 echo $cmd
 $cmd
 if [ ! $? = 0 ]; then
  echo error running xgettext
  exit 1
 fi
 sed -i s/"CHARSET"/"UTF-8"/g ./po/openscad.pot
}

updatepo()
{
 for LANGCODE in `cat ./po/LINGUAS | grep -v "#"`; do
	OPTS=
  OPTS='--update --backup=t'
  cmd='msgmerge '$OPTS' ./po/'$LANGCODE'.po ./po/openscad.pot'
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
  cmd='msgfmt -c -v -o ./po/'$LANGCODE'/LC_MESSAGES/openscad.mo ./po/'$LANGCODE'.po'
  echo $cmd
  $cmd
  if [ ! $? = 0 ]; then
   echo error running msgfmt
   exit 1
  fi
 done
}

if [ ! $1 ] ; then
 help
 exit 0
fi

if [ $1 = updatepot ]; then
 updatepot
elif [ $1 = updatepo ]; then
 updatepo
elif [ $1 = updatemo ]; then
 updatemo
elif [ $1 = updateall ]; then
 updatepot && updatepo && updatemo
fi

