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
#
# todo - detect failure and stop

DRYRUN=

check_starting_path()
{
	STARTPATH=$PWD
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
	# 1=file summary, 2 = username, 3 = filename
	if [ -e $3 ]; then
		echo $3 found
	else
		echo $3 not found
	fi
	opts=
	opts="$opts -p openscad"
	opts="$opts -u $2"
	opts="$opts $3"
	if [ ! $DRYRUN ]; then
		python ./scripts/googlecode_upload.py -s "$1" $opts
	else
		echo dry run, not uploading to googlecode
	fi
}

upload_win32()
{
	SUMMARY1="Windows x86-32 Snapshot Installer"
	SUMMARY2="Windows x86-32 Snapshot Zipfile"
	DATECODE=`date +"%Y.%m.%d"`
	BASEDIR=./mingw32/
	WIN32_PACKAGEFILE1=OpenSCAD-$DATECODE-x86-32-Installer.zip
	WIN32_PACKAGEFILE2=OpenSCAD-$DATECODE-x86-32.exe
	upload_win_generic "$SUMMARY1" $USERNAME $BASEDIR/$WIN32_PACKAGEFILE1
	upload_win_generic "$SUMMARY2" $USERNAME $BASEDIR/$WIN32_PACKAGEFILE2
	export WIN32_PACKAGEFILE1
	export WIN32_PACKAGEFILE2
	WIN32_PACKAGEFILE1_SIZE=`ls -sh $BASEDIR/$WIN32_PACKAGEFILE1 | awk ' {print $1} ';`
	WIN32_PACKAGEFILE2_SIZE=`ls -sh $BASEDIR/$WIN32_PACKAGEFILE2 | awk ' {print $1} ';`
	WIN32_PACKAGEFILE1_SIZE=`echo "$WIN32_PACKAGEFILE1_SIZE""B"`
	WIN32_PACKAGEFILE2_SIZE=`echo "$WIN32_PACKAGEFILE2_SIZE""B"`
	export WIN32_PACKAGEFILE1_SIZE
	export WIN32_PACKAGEFILE2_SIZE
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

update_www_download_links()
{
	cd $STARTPATH
	git clone git@github.com:openscad/openscad.github.com.git
	cd openscad.github.com
	cd inc
	echo `pwd`
	BASEURL='https://openscad.googlecode.com/files/'
	DATECODE=`date +"%Y.%m.%d"`

	rm win_snapshot_links.js
	echo "snapinfo['WIN32_SNAPSHOT1_URL'] = '$BASEURL$WIN32_PACKAGEFILE1'" >> win_snapshot_links.js
	echo "snapinfo['WIN32_SNAPSHOT2_URL'] = '$BASEURL$WIN32_PACKAGEFILE2'" >> win_snapshot_links.js
	echo "snapinfo['WIN32_SNAPSHOT1_NAME'] = 'OpenSCAD $DATECODE'" >> win_snapshot_links.js
	echo "snapinfo['WIN32_SNAPSHOT2_NAME'] = 'OpenSCAD $DATECODE'" >> win_snapshot_links.js
	echo "snapinfo['WIN32_SNAPSHOT1_SIZE'] = '$WIN32_PACKAGEFILE1_SIZE'" >> win_snapshot_links.js
	echo "snapinfo['WIN32_SNAPSHOT2_SIZE'] = '$WIN32_PACKAGEFILE2_SIZE'" >> win_snapshot_links.js
	echo 'modified win_snapshot_links.js'
	cat win_snapshot_links.js

	PAGER=cat git diff
	if [ ! $DRYRUN ]; then
		git commit -a -m 'builder.sh - updated snapshot links'
		git push origin
	else
		echo dry run, not updating www links
	fi
}

check_ssh_agent()
{
	if [ ! $SSH_AUTH_SOCK ]; then
		echo 'please start an ssh-agent for github.com/openscad/openscad.github.com uploads'
		echo 'for example:'
		echo
		echo ' ssh-agent > .tmp && source .tmp && ssh-add'
		echo
	fi
}

check_ssh_agent
check_starting_path
read_username_from_user
read_password_from_user
get_source_code
build_win32
upload_win32
update_www_download_links



