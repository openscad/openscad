#!/bin/bash

dmgfile=$1
if [ -z "$dmgfile" ]; then
  echo "Usage: $0 <dmgfile>"
  exit 1
fi
indexfile=../openscad.github.com/index.html
if [ -f $indexfile ]; then
  sed -i .backup -e "s/^\(.*mac-snapshot.*\)\(openscad-.*\.dmg\)\(.*\)\(openscad-.*dmg\)\(.*$\)/\\1$dmgfile\\3$dmgfile\\5/" $indexfile
else
  echo "Web page not found at $indexfile"
fi
echo "Web page updated. Remember to commit and push openscad.githib.com"
