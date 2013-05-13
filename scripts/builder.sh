#!/usr/bin/env bash

# build&upload script for linux & windows snapshot binaries
# tested under linux

# requirements -
# see http://mxe.cc for required tools (scons, perl, yasm, etc etc etc)

# todo - auto update webpage to link to proper snapshot
#
# todo - 64 bit windows (needs mxe 64 bit stable)
#
# todo - can we build 32 bit linux from within 64 bit linux?
#
# todo - make linux work

check_starting_path()
{
	if [ -e openscad.pro ]; then
		echo 'please start from a clean directory outside of openscad'
		exit
	fi
}

get_source_code()
{
	git clone http://github.com/openscad/openscad.git
	cd openscad
	git submodule update --init # MCAD
}

build_win32()
{
	. ./scripts/setenv-mingw-xbuild.sh
	./scripts/mingw-x-build-dependencies.sh
	./scripts/release-common.sh mingw32
}

build_lin32()
{
	. ./scripts/setenv-unibuild.sh clang
	./scripts/uni-build-dependencies.sh
	./scripts/release-common.sh
}

upload_win_generic()
{
	if [ -e $3 ]; then
		echo $3 found
	else
		echo $3 not found
	fi
	opts=
	opts="$opts -p openscad"
	opts="$opts -u $2"
	opts="$opts $3"
	echo python ./scripts/googlecode_upload.py -s "$1" $opts
	python ./scripts/googlecode_upload.py -s "$1" $opts
}

upload_win32()
{
	SUMMARY1="Windows x86-32 Snapshot Zipfile"
	SUMMARY2="Windows x86-32 Snapshot Installer"
	DATECODE=`date +"%Y.%m.%d"`
	PACKAGEFILE1=./mingw32/OpenSCAD-$DATECODE-x86-32.zip
	PACKAGEFILE2=./mingw32/OpenSCAD-$DATECODE-x86-32-Installer.exe
	upload_win_generic "$SUMMARY1" $USERNAME $PACKAGEFILE1
	upload_win_generic "$SUMMARY2" $USERNAME $PACKAGEFILE2
}

read_username_from_user()
{
	echo 'Please enter your username for https://code.google.com/hosting/settings'
	echo -n 'Username:'
	read USERNAME
	echo 'username is ' $USERNAME
}

read_password_from_user()
{
	echo 'Please enter your password for https://code.google.com/hosting/settings'
	echo -n 'Password:'
	read -s PASSWORD1
	echo
	echo -n 'Verify  :'
	read -s PASSWORD2
	echo
	if [ ! $PASSWORD1 = $PASSWORD2 ]; then
		echo 'error - passwords dont match'
		exit
	fi
	OSUPL_PASSWORD=$PASSWORD1
	export OSUPL_PASSWORD
}

check_starting_path
read_username_from_user
read_password_from_user
get_source_code
build_win32
upload_win32



