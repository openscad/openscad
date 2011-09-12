#!/bin/bash

dmgfile=$1
if [ -z "$dmgfile" ]; then
  echo "Usage: $0 <dmgfile>"
  exit 1
fi
indexfile=../openscad.github.com/index.html
if [ -f $indexfile ]; then
  sed -i .backup -e "s/^\(.*mac-snapshot.*\)\(OpenSCAD-.*\.dmg\)\(.*\)\(OpenSCAD-.*dmg\)\(.*$\)/\\1$dmgfile\\3$dmgfile\\5/" $indexfile
  echo "Web page updated. Remember to commit and push openscad.github.com"
else
  echo "Web page not found at $indexfile"
fi
