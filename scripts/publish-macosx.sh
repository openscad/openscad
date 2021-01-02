#!/bin/sh
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
SHORTVERSION=${VERSION%%-*}

# Turn off ccache, just for safety
PATH=${PATH//\/opt\/local\/libexec\/ccache:}

export MACOSX_DEPLOYMENT_TARGET=10.9

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

SIGNATURE=$(openssl dgst -sha1 -binary < OpenSCAD-$VERSION.dmg  | openssl dgst -dss1 -sign $HOME/.ssh/openscad-appcast.pem | openssl enc -base64)

if [[ $VERSION == $VERSIONDATE ]]; then
  APPCASTFILE=appcast-snapshots.xml
else
  APPCASTFILE=appcast.xml
fi
echo "Creating appcast $APPCASTFILE..."
FILESIZE=$(stat -f "%z" OpenSCAD-$VERSION.dmg)
sed -e "s,@VERSION@,$VERSION,g" -e "s,@SHORTVERSION@,$SHORTVERSION,g" -e "s,@VERSIONDATE@,$VERSIONDATE,g" -e "s,@DSASIGNATURE@,$SIGNATURE,g" -e "s,@FILESIZE@,$FILESIZE,g" $APPCASTFILE.in > $APPCASTFILE
cp $APPCASTFILE ../openscad.github.com
if [[ $VERSION == $VERSIONDATE ]]; then
  cp $APPCASTFILE ../openscad.github.com/appcast-snapshots.xml
fi

if [ ! $DOUPLOAD ]; then
  exit 0
fi

echo "Uploading..."
if [[ $VERSION == $VERSIONDATE ]]; then
  scp OpenSCAD-$VERSION.dmg openscad@files.openscad.org:www/snapshots
else
  scp OpenSCAD-$VERSION.dmg openscad@files.openscad.org:www
fi
if [[ $? != 0 ]]; then
  exit 1
fi
scp $APPCASTFILE openscad@files.openscad.org:www/
if [[ $? != 0 ]]; then
  exit 1
fi
if [[ $VERSION == $VERSIONDATE ]]; then
  scp $APPCASTFILE openscad@files.openscad.org:www/appcast-snapshots.xml
  if [[ $? != 0 ]]; then
    exit 1
  fi
fi

# Update snapshot filename on web page
update_www_download_links version=$VERSION packagefile=OpenSCAD-$VERSION.dmg filesize=$FILESIZE
