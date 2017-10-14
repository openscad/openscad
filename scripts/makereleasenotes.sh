#!/bin/sh

echo "" > RELEASE_NOTES.md
for v in `ls -r releases/*.md`; do
  mdfile=${v#releases/}
  version=${mdfile%.md}
  echo "# OpenSCAD $version\n" >> RELEASE_NOTES.md
  cat $v >> RELEASE_NOTES.md
done
