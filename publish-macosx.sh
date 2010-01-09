#!/bin/sh

VERSION=`date "+%Y.%m.%d"`
#VERSION=2010.02

./release-macosx.sh -v $VERSION
cp openscad-$VERSION.zip ~/Documents/Dropbox/Public
ln -sf openscad-$VERSION.zip ~/Documents/Dropbox/Public/openscad-latest.zip 
