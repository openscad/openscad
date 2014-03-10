#!/bin/sh

echo "" > RELEASE_NOTES
for v in `ls -r releases/*.md`; do
  mdfile=${v#releases/}
  version=${mdfile%.md}
  echo "# OpenSCAD $version\n" >> RELEASE_NOTES
  cat $v >> RELEASE_NOTES
done
