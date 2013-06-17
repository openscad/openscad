#!/usr/bin/env bash

# build&upload script for linux & windows snapshot binaries
# tested under linux

# requirements -
# see http://mxe.cc for required tools (scons, perl, yasm, etc etc etc)

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
	STARTPATH=$PWD
	export STARTPATH
	if [ "`echo $* | grep uploadonly`" ]; then
		UPLOADONLY=1
		DATECODE=`date +"%Y.%m.%d"`
	else
		UPLOADONLY=
	fi
	if [ "`echo $* | grep dry`" ]; then
		DRYRUN=1
	else
		DRYRUN=
	fi
	export UPLOADONLY
	export DRYRUN
	export DATECODE
}

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
	#git checkout branch ##debugging
}

build_win32()
{
	. ./scripts/setenv-mingw-xbuild.sh clean
	. ./scripts/setenv-mingw-xbuild.sh
	./scripts/mingw-x-build-dependencies.sh
	./scripts/release-common.sh mingw32
	DATECODE=`date +"%Y.%m.%d"`
	export DATECODE
}

build_win64()
{
	. ./scripts/setenv-mingw-xbuild.sh clean
	. ./scripts/setenv-mingw-xbuild.sh 64
	./scripts/mingw-x-build-dependencies.sh 64
	./scripts/release-common.sh mingw64
	DATECODE=`date +"%Y.%m.%d"`
	export DATECODE
}

build_lin32()
{
	. ./scripts/setenv-unibuild.sh
	./scripts/uni-build-dependencies.sh
	./scripts/release-common.sh
	DATECODE=`date +"%Y.%m.%d"`
	export DATECODE
}

upload_win_generic()
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
		echo dry run, not uploading to googlecode
		echo cmd - python ./scripts/googlecode_upload.py -s '"'$summary'"' $opts
	else
		python ./scripts/googlecode_upload.py -s "$summary" $opts
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

upload_win64()
{
	SUMMARY1="Windows x86-64 Snapshot Zipfile"
	SUMMARY2="Windows x86-64 Snapshot Installer"
	BASEDIR=./mingw64/
	WIN64_PACKAGEFILE1=OpenSCAD-$DATECODE-x86-64-Installer.exe
	WIN64_PACKAGEFILE2=OpenSCAD-$DATECODE-x86-64.zip
	upload_win_generic "$SUMMARY1" $USERNAME $BASEDIR/$WIN64_PACKAGEFILE1
	upload_win_generic "$SUMMARY2" $USERNAME $BASEDIR/$WIN64_PACKAGEFILE2
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
	echo 'Please enter your username for https://code.google.com/hosting/settings'
	echo -n 'Username:'
	read USERNAME
	echo 'username is ' $USERNAME
}

read_password_from_user()
{
	if [ $DRYRUN ]; then return; fi
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
	BASEURL='https://openscad.googlecode.com/files/'
	DATECODE=`date +"%Y.%m.%d"`

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
	if [ ! $SSH_AUTH_SOCK ]; then
		echo 'please start an ssh-agent for github.com/openscad/openscad.github.com uploads'
		echo 'for example:'
		echo
		echo ' ssh-agent > .tmp && source .tmp && ssh-add'
		echo
	fi
}

init_variables $*
check_ssh_agent
check_starting_path
read_username_from_user
read_password_from_user
get_source_code

if [ ! $UPLOADONLY ]; then
	build_win32
	build_win64
fi
upload_win32
upload_win64
update_win_www_download_links



