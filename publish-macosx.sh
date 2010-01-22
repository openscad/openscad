#!/bin/sh

VERSION=`date "+%Y.%m.%d"`
#VERSION=2010.02

export OPENCSGDIR=$PWD/../OpenCSG-1.2.0

./release-common.sh -v $VERSION
cp openscad-$VERSION.zip ~/Documents/Dropbox/Public
ln -sf openscad-$VERSION.zip ~/Documents/Dropbox/Public/openscad-latest.zip 
