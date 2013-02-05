#!/bin/sh

# NB! To build a release build, the VERSION and VERSIONDATE environment variables needs to be set.
# See doc/release-checklist.txt

if test -z "$VERSIONDATE"; then
  VERSIONDATE=`date "+%Y.%m.%d"`
fi
if test -z "$VERSION"; then
  VERSION=$VERSIONDATE
  COMMIT=-c
  SNAPSHOT=true
fi

# Turn off ccache, just for safety
PATH=${PATH//\/opt\/local\/libexec\/ccache:}

# This is the same location as DEPLOYDIR in macosx-build-dependencies.sh
export OPENSCAD_LIBRARIES=$PWD/../libraries/install

# Make sure that the correct Qt tools are used
export PATH=$OPENSCAD_LIBRARIES/bin:$PATH

`dirname $0`/release-common.sh -v $VERSION $COMMIT
if [[ $? != 0 ]]; then
  exit 1
fi

echo "Sanity check of the app bundle..."
`dirname $0`/macosx-sanity-check.py OpenSCAD.app/Contents/MacOS/OpenSCAD
if [[ $? != 0 ]]; then
  exit 1
fi

if [[ $VERSION == $VERSIONDATE ]]; then
  APPCASTFILE=appcast-snapshots.xml
else
  APPCASTFILE=appcast.xml
fi
echo "Creating appcast $APPCASTFILE..."
sed -e "s,@VERSION@,$VERSION,g" -e "s,@VERSIONDATE@,$VERSIONDATE,g" -e "s,@FILESIZE@,$(stat -f "%z" OpenSCAD-$VERSION.dmg),g" $APPCASTFILE.in > $APPCASTFILE
cp $APPCASTFILE ../openscad.github.com
if [[ $VERSION == $VERSIONDATE ]]; then
  cp $APPCASTFILE ../openscad.github.com/appcast-snapshots.xml
fi

echo "Uploading..."
#LABELS=OpSys-OSX,Type-Executable
#if ! $SNAPSHOT; then LABELS=$LABELS,Featured; fi
#`dirname $0`/googlecode_upload.py -s 'Mac OS X Snapshot' -p openscad OpenSCAD-$VERSION.dmg -l $LABELS

# Update snapshot filename on web page
#`dirname $0`/update-web.sh OpenSCAD-$VERSION.dmg
