#!/usr/bin/env bash

# build&upload script for linux & windows snapshot binaries
#
# Usage:
#
# Start with a clean directory. For example:
#
# mkdir builder
# cd builder
#
# Then run this script, or optionally the 'build only' or 'upload only' version
#
# /some/path/openscad/builder.sh            # standard build & upload
# /some/path/openscad/builder.sh buildonly  # only do build, dont upload
# /some/path/openscad/builder.sh uploadonly # only upload, dont build

# Notes:
#
# This script is designed to build a 'clean' version of openscad directly
# from the openscad github master source code. It will then optionally
# upload the build to the OpenSCAD official file repository, and modify
# the OpenSCAD website with links to the most recently built files.
#
#
# For the mingw- cross build for Windows(TM) this script does a massive
# 'from nothing' build, including downloading and building an MXE cross
# environment, and dependencies, into $HOME/openscad_deps. This can take
# many many many hours and use several gigabytes of disk space.
#
# This script itself is designed to call other scripts that do the heavy
# lifting. This script itself should be kept relatively simple.
#

#
# requirements -
# see http://mxe.cc for required tools (scons, perl, yasm, etc etc etc)
#
# todo - can we build 32 bit linux from within 64 bit linux?
#
# todo - make linux work
#
# todo - detect failure and stop
#
# todo - generalize to build release binaries as well
#

init_variables()
{
	BRANCH_TO_BUILD=unstable
	#BRANCH_TO_BUILD=unstable
	STARTPATH=$PWD
	export STARTPATH
	DOBUILD=1
	DOUPLOAD=1
	DRYRUN=
	DOSNAPSHOT=1
	if [ "`echo $* | grep release`" ]; then
		echo "this script cannot yet build releases, only snapshots"
		DOSNAPSHOT=0
		exit 1
	fi
	if [ "`echo $* | grep uploadonly`" ]; then
		DOUPLOAD=1
		DOBUILD=
		DATECODE=`date +"%Y.%m.%d"`
	fi
	if [ "`echo $* | grep buildonly`" ]; then
		DOUPLOAD=
		DOBUILD=1
		DATECODE=`date +"%Y.%m.%d"`
	fi
	if [ "`echo $* | grep dry`" ]; then
		DRYRUN=1
	fi
	export BRANCH_TO_BUILD
	export DOBUILD
	export DOUPLOAD
	export DRYRUN
	export DATECODE
	export DOSNAPSHOT
}

check_starting_path()
{
	if [ -e openscad.pro ]; then
		echo 'please start from a clean directory outside of openscad'
		exit
	fi
}

check_nsis()
{
	# 64 bit mingw-cross build MXE cannot build nsis.... for now, we can
	# just ask the user to install their system's nsis package.
	# (it might be possible to d/l & build nsis here, or use pre-existing
	#  32-bit-mxe nsis)
	if [ ! "`command -v makensis`" ]; then
		echo the makensis command was not found.
		echo please install nsis for your system. for example
		echo on debian, sudo apt-get install nsis
		exit 1
	else
		echo makensis found.
	fi
}

get_openscad_source_code()
{
	if [ -d openscad ]; then
		cd openscad
		if [ ! $? ]; then
			echo cd to 'openscad' directory failed
			exit 1
		fi
		git checkout $BRANCH_TO_BUILD
		if [ ! $? ]; then
			echo git checkout $BRANCH_TO_BUILD failed
			exit 1
		fi
		git fetch -a
		if [ ! $? ]; then
			echo git fetch -a openscad source code failed
			exit 1
		fi
		git pull origin $BRANCH_TO_BUILD
		if [ ! $? ]; then
			echo git pull origin $BRANCH_TO_BUILD failed
			exit 1
		fi
		git submodule update # MCAD
		return
	fi
	git clone http://github.com/openscad/openscad.git
	if [ $? ]; then
		echo clone of source code is ok
	else
		if [ $DOUPLOAD ]; then
			if [ ! $DOBUILD ]; then
				echo upload only - skipping openscad git clone
			fi
		else
			echo clone of openscad source code failed. exiting
			exit 1
		fi
	fi
	cd openscad
	git checkout $BRANCH_TO_BUILD
	if [ ! $? ]; then
		echo git checkout $BRANCH_TO_BUILD failed
		exit 1
	fi
	git submodule update --init # MCAD
}

build_win32()
{
	. ./scripts/setenv-mingw-xbuild.sh clean
	. ./scripts/setenv-mingw-xbuild.sh
	./scripts/mingw-x-build-dependencies.sh
	if [ $DOSNAPSHOT ] ; then
		./scripts/release-common.sh snapshot mingw32
	else
		echo "this script cant yet build releases, only snapshots"
		exit 1
	fi
	if [ "`echo $? | grep 0`" ]; then
		echo build of win32 stage over
	else
		echo build of win32 failed. exiting
		exit 1
	fi
	DATECODE=`date +"%Y.%m.%d"`
	export DATECODE
}

build_win64()
{
	. ./scripts/setenv-mingw-xbuild.sh clean
	. ./scripts/setenv-mingw-xbuild.sh 64
	./scripts/mingw-x-build-dependencies.sh 64
	if [ $DOSNAPSHOT ] ; then
		./scripts/release-common.sh snapshot mingw64
	else
		echo "this script cant yet build releases, only snapshots"
		exit 1
	fi
	if [ "`echo $? | grep 0`" ]; then
		echo build of win64 stage over
	else
		echo build of win64 failed. exiting
		exit 1
	fi
	DATECODE=`date +"%Y.%m.%d"`
	export DATECODE
}

build_lin32()
{
	. ./scripts/setenv-unibuild.sh
	./scripts/uni-build-dependencies.sh
	if [ $DOSNAPSHOT ] ; then
		./scripts/release-common.sh snapshot
	else
		echo "this script cant yet build releases, only snapshots"
		exit 1
	fi
	DATECODE=`date +"%Y.%m.%d"`
	export DATECODE
}

upload_win_common()
{
	summary="$1"
	username=$2
	filename=$3
	if [ -f $filename ]; then
		echo 'file "'$filename'" found'
	else
		echo 'file "'$filename'" not found'
	fi
	opts=
	opts="$opts -p openscad"
	opts="$opts -u $username"
	opts="$opts $filename"
	if [ $DRYRUN ]; then
		echo dry run, not uploading to files.openscad.org
		echo scp -v $filename openscad@files.openscad.org:www/
	else
		scp -v $filename openscad@files.openscad.org:www/
	fi
}

upload_win32()
{
	SUMMARY1="Windows x86-32 Snapshot Installer"
	SUMMARY2="Windows x86-32 Snapshot Zipfile"
	DATECODE=`date +"%Y.%m.%d"`
	BASEDIR=./mingw32/
	WIN32_PACKAGEFILE1=OpenSCAD-$DATECODE-x86-32-Installer.exe
	WIN32_PACKAGEFILE2=OpenSCAD-$DATECODE-x86-32.zip
	upload_win_common "$SUMMARY1" $USERNAME $BASEDIR/$WIN32_PACKAGEFILE1
	upload_win_common "$SUMMARY2" $USERNAME $BASEDIR/$WIN32_PACKAGEFILE2
	export WIN32_PACKAGEFILE1
	export WIN32_PACKAGEFILE2
	WIN32_PACKAGEFILE1_SIZE=`ls -sh $BASEDIR/$WIN32_PACKAGEFILE1 | awk ' {print $1} ';`
	WIN32_PACKAGEFILE2_SIZE=`ls -sh $BASEDIR/$WIN32_PACKAGEFILE2 | awk ' {print $1} ';`
	WIN32_PACKAGEFILE1_SIZE=`echo "$WIN32_PACKAGEFILE1_SIZE""B"`
	WIN32_PACKAGEFILE2_SIZE=`echo "$WIN32_PACKAGEFILE2_SIZE""B"`
	export WIN32_PACKAGEFILE1_SIZE
	export WIN32_PACKAGEFILE2_SIZE
}

upload_win64()
{
	SUMMARY1="Windows x86-64 Snapshot Zipfile"
	SUMMARY2="Windows x86-64 Snapshot Installer"
	BASEDIR=./mingw64/
	WIN64_PACKAGEFILE1=OpenSCAD-$DATECODE-x86-64-Installer.exe
	WIN64_PACKAGEFILE2=OpenSCAD-$DATECODE-x86-64.zip
	upload_win_common "$SUMMARY1" $USERNAME $BASEDIR/$WIN64_PACKAGEFILE1
	upload_win_common "$SUMMARY2" $USERNAME $BASEDIR/$WIN64_PACKAGEFILE2
	export WIN64_PACKAGEFILE1
	export WIN64_PACKAGEFILE2
	WIN64_PACKAGEFILE1_SIZE=`ls -sh $BASEDIR/$WIN64_PACKAGEFILE1 | awk ' {print $1} ';`
	WIN64_PACKAGEFILE2_SIZE=`ls -sh $BASEDIR/$WIN64_PACKAGEFILE2 | awk ' {print $1} ';`
	WIN64_PACKAGEFILE1_SIZE=`echo "$WIN64_PACKAGEFILE1_SIZE""B"`
	WIN64_PACKAGEFILE2_SIZE=`echo "$WIN64_PACKAGEFILE2_SIZE""B"`
	export WIN64_PACKAGEFILE1_SIZE
	export WIN64_PACKAGEFILE2_SIZE
}

read_username_from_user()
{
	if [ $DRYRUN ]; then USERNAME=none;export USERNAME; return; fi
	echo 'Google code upload is deprecated'
	USERNAME=$USER
	echo 'username is ' $USERNAME
	return

	echo 'Please enter your username for https://code.google.com/hosting/settings'
	echo -n 'Username:'
	read USERNAME
	echo 'username is ' $USERNAME
	return
}

read_password_from_user()
{
	if [ $DRYRUN ]; then return; fi
	echo 'Google code upload is deprecated'
	return

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

update_win_www_download_links()
{
	cd $STARTPATH
	git clone git@github.com:openscad/openscad.github.com.git
	cd openscad.github.com
	cd inc
	echo `pwd`
	# BASEURL='https://openscad.googlecode.com/files/'
	BASEURL='http://files.openscad.org/'
	DATECODE=`date +"%Y.%m.%d"`

	mv win_snapshot_links.js win_snapshot_links.js.backup
	rm win_snapshot_links.js
	echo "fileinfo['WIN64_SNAPSHOT1_URL'] = '$BASEURL$WIN64_PACKAGEFILE1'" >> win_snapshot_links.js
	echo "fileinfo['WIN64_SNAPSHOT2_URL'] = '$BASEURL$WIN64_PACKAGEFILE2'" >> win_snapshot_links.js
	echo "fileinfo['WIN64_SNAPSHOT1_NAME'] = 'OpenSCAD $DATECODE'" >> win_snapshot_links.js
	echo "fileinfo['WIN64_SNAPSHOT2_NAME'] = 'OpenSCAD $DATECODE'" >> win_snapshot_links.js
	echo "fileinfo['WIN64_SNAPSHOT1_SIZE'] = '$WIN64_PACKAGEFILE1_SIZE'" >> win_snapshot_links.js
	echo "fileinfo['WIN64_SNAPSHOT2_SIZE'] = '$WIN64_PACKAGEFILE2_SIZE'" >> win_snapshot_links.js

	echo "fileinfo['WIN32_SNAPSHOT1_URL'] = '$BASEURL$WIN32_PACKAGEFILE1'" >> win_snapshot_links.js
	echo "fileinfo['WIN32_SNAPSHOT2_URL'] = '$BASEURL$WIN32_PACKAGEFILE2'" >> win_snapshot_links.js
	echo "fileinfo['WIN32_SNAPSHOT1_NAME'] = 'OpenSCAD $DATECODE'" >> win_snapshot_links.js
	echo "fileinfo['WIN32_SNAPSHOT2_NAME'] = 'OpenSCAD $DATECODE'" >> win_snapshot_links.js
	echo "fileinfo['WIN32_SNAPSHOT1_SIZE'] = '$WIN32_PACKAGEFILE1_SIZE'" >> win_snapshot_links.js
	echo "fileinfo['WIN32_SNAPSHOT2_SIZE'] = '$WIN32_PACKAGEFILE2_SIZE'" >> win_snapshot_links.js
	echo 'modified win_snapshot_links.js'

	PAGER=cat git diff
	if [ ! $DRYRUN ]; then
		git commit -a -m 'builder.sh - updated snapshot links'
		git push origin master
	else
		echo dry run, not updating www links
	fi
}

# FIXME: We might be running this locally and not need an ssh agent.
# Before checking $SSH_AUTH_SOCK, try 'ssh -T git@github.com' to verify that we
# can access github over ssh
check_ssh_agent()
{
	if [ $DRYRUN ]; then echo 'skipping ssh, dry run'; return; fi
	if [ $SSH_AUTH_SKIP ]; then
		return
	fi
	if [ ! $SSH_AUTH_SOCK ]; then
		echo 'please start an ssh-agent for github.com/openscad/openscad.github.com uploads'
		echo 'for example:'
		echo
		echo ' ssh-agent > .tmp && source .tmp && ssh-add'
		echo
		echo 'to force a run anyway, set SSH_AUTH_SKIP environment variable to 1'
		exit 1
	fi
}

init_variables $*
if [ $DOUPLOAD ]; then
	check_ssh_agent
fi
check_starting_path
check_nsis
read_username_from_user
read_password_from_user
get_openscad_source_code
if [ $DOBUILD ]; then
	build_win32
	build_win64
fi
if [ $DOUPLOAD ]; then
	upload_win32
	upload_win64
	update_win_www_download_links
fi

