#!/bin/sh

# NB! To build a release build, the VERSION and VERSIONDATE environment variables needs to be set.
# See doc/release-checklist.txt

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
    incfile=inc/mac_snapshot_links.js
    BASEURL='https://openscad.googlecode.com/files/'
    DATECODE=`date +"%Y.%m.%d"`
    
    if [ -f $webdir/$incfile ]; then
        cd $webdir
        echo "fileinfo['MAC_SNAPSHOT_URL'] = '$BASEURL$packagefile'" > $incfile
        echo "fileinfo['MAC_SNAPSHOT_NAME'] = 'OpenSCAD $version'" >> $incfile
        echo "fileinfo['MAC_SNAPSHOT_SIZE'] = '$filesize'" >> $incfile
        echo 'modified mac_snapshot_links.js'
        
        git --no-pager diff
        echo "Web page updated. Remember to commit and push openscad.github.com"
    else
        echo "Web page not found at $incfile"
    fi
}

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

SIGNATURE=$(openssl dgst -sha1 -binary < OpenSCAD-$VERSION.dmg  | openssl dgst -dss1 -sign dsa_priv.pem | openssl enc -base64)

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
LABELS=OpSys-OSX,Type-Executable
if ! $SNAPSHOT; then LABELS=$LABELS,Featured; fi
`dirname $0`/googlecode_upload.py -s 'Mac OS X Snapshot' -p openscad OpenSCAD-$VERSION.dmg -l $LABELS
if [[ $? != 0 ]]; then
  exit 1
fi

scp OpenSCAD-$VERSION.dmg openscad@files.openscad.org:www
if [[ $? != 0 ]]; then
  exit 1
fi

# Update snapshot filename on web page
update_www_download_links version=$VERSION packagefile=OpenSCAD-$VERSION.dmg filesize=$FILESIZE
