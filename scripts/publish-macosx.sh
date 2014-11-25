#!/bin/sh
# NB! To build a release build, the VERSION and VERSIONDATE environment variables needs to be set.
# See doc/release-checklist.txt

#
# Usage:
#   ./scripts/publish-macosx.sh [buildonly]
#

export NUMCPU=$(sysctl -n hw.ncpu)

human_filesize()
{
  awk -v sum=$1 'BEGIN {
    hum[1024**3]="GB"; hum[1024**2]="MB"; hum[1024]="KB"; 
    for (x=1024**3; x>=1024; x/=1024) { 
      if (sum>=x) { printf "%.1f %s\n",sum/x,hum[x]; break }
    }
  }'
}

# Pass version=<version> packagefile=<packagefile> filesize=<bytes>
update_www_download_links()
{
    # Make the passed variables available
    local $*
    filesize=$(human_filesize $filesize)
    webdir=../openscad.github.com
    # FIXME: release vs. snapshot
    incfile=inc/mac_snapshot_links.js
    BASEURL='http://files.openscad.org/snapshots'
    DATECODE=`date +"%Y.%m.%d"`
    
    if [ -f $webdir/$incfile ]; then
        cd $webdir
        echo "fileinfo['MAC_SNAPSHOT_URL'] = '$BASEURL/$packagefile'" > $incfile
        echo "fileinfo['MAC_SNAPSHOT_NAME'] = 'OpenSCAD $version'" >> $incfile
        echo "fileinfo['MAC_SNAPSHOT_SIZE'] = '$filesize'" >> $incfile
        echo 'modified mac_snapshot_links.js'
        
        git --no-pager diff
        echo "Web page updated. Remember to commit and push openscad.github.com"
    else
        echo "Web page not found at $incfile"
    fi
}

# Cmd-line parameters
DOUPLOAD=1
if [ "`echo $* | grep buildonly`" ]; then
  echo "Build only, no upload"
  DOUPLOAD=
fi

if test -z "$VERSIONDATE"; then
  VERSIONDATE=`date "+%Y.%m.%d"`
fi
if test -z "$VERSION"; then
  VERSION=$VERSIONDATE
  SNAPSHOT=snapshot
fi

# Turn off ccache, just for safety
PATH=${PATH//\/opt\/local\/libexec\/ccache:}

# FIXME: Somehow, setting the deployment target to a lower version causes a
# seldom crash in debug mode (e.g. the minkowski2-test):
# frame #4: 0x00007fff8b7d5be5 libc++.1.dylib`std::runtime_error::~runtime_error() + 55
# frame #5: 0x0000000100150df5 OpenSCAD`CGAL::Uncertain_conversion_exception::~Uncertain_conversion_exception(this=0x0000000105044488) + 21 at Uncertain.h:78
# The reason for the crash appears to be linking with libgcc_s, 
# but it's unclear what's really going on
export MACOSX_DEPLOYMENT_TARGET=10.6

# This is the same location as DEPLOYDIR in macosx-build-dependencies.sh
export OPENSCAD_LIBRARIES=$PWD/../libraries/install

# Make sure that the correct Qt tools are used
export PATH=$OPENSCAD_LIBRARIES/bin:$PATH

`dirname $0`/release-common.sh -v $VERSION $SNAPSHOT
if [[ $? != 0 ]]; then
  exit 1
fi

echo "Sanity check of the app bundle..."
`dirname $0`/macosx-sanity-check.py OpenSCAD.app/Contents/MacOS/OpenSCAD
if [[ $? != 0 ]]; then
  exit 1
fi

if [ ! $DOUPLOAD ]; then
  exit 0
fi

SIGNATURE=$(openssl dgst -sha1 -binary < OpenSCAD-$VERSION.dmg  | openssl dgst -dss1 -sign $HOME/.ssh/openscad-appcast.pem | openssl enc -base64)

if [[ $VERSION == $VERSIONDATE ]]; then
  APPCASTFILE=appcast-snapshots.xml
else
  APPCASTFILE=appcast.xml
fi
echo "Creating appcast $APPCASTFILE..."
FILESIZE=$(stat -f "%z" OpenSCAD-$VERSION.dmg)
sed -e "s,@VERSION@,$VERSION,g" -e "s,@VERSIONDATE@,$VERSIONDATE,g" -e "s,@DSASIGNATURE@,$SIGNATURE,g" -e "s,@FILESIZE@,$FILESIZE,g" $APPCASTFILE.in > $APPCASTFILE
cp $APPCASTFILE ../openscad.github.com
if [[ $VERSION == $VERSIONDATE ]]; then
  cp $APPCASTFILE ../openscad.github.com/appcast-snapshots.xml
fi

echo "Uploading..."
scp OpenSCAD-$VERSION.dmg openscad@files.openscad.org:www/snapshots
if [[ $? != 0 ]]; then
  exit 1
fi

# Update snapshot filename on web page
update_www_download_links version=$VERSION packagefile=OpenSCAD-$VERSION.dmg filesize=$FILESIZE
