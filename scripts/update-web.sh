#!/usr/bin/env bash

# Works with Mac OS X and Linux cross-compiling for windows using
# mingw-cross-env (use like: OS=LINXWIN update-web.sh file1.zip file2.exe).

file1=$1
if [ -z "$file1" ]; then
  echo "Usage: $0 <file1> [<file2>]"
  exit 1
fi

if [[ $OSTYPE =~ "darwin" ]]; then
  OS=MACOSX
elif [[ $OSTYPE == "linux-gnu" ]]; then
  OS=LINUX
fi

indexfile=../openscad.github.com/index.html
if [ -f $indexfile ]; then
  if [ $OS == MACOSX ]; then
    sed -i .backup -e "s/^\(.*mac-snapshot.*\)\(OpenSCAD-.*\.dmg\)\(.*\)\(OpenSCAD-.*dmg\)\(.*$\)/\\1$file1\\3$file1\\5/" $indexfile
  elif [ $OS == LINXWIN ]; then
    file2=$2
    sed -i .backup -e "s/^\(.*win-snapshot-zip.*\)\(OpenSCAD-.*\.zip\)\(.*\)\(OpenSCAD-.*zip\)\(.*$\)/\\1$file1\\3$file1\\5/" $indexfile
    sed -i .backup -e "s/^\(.*win-snapshot-exe.*\)\(OpenSCAD-.*-Installer\.exe\)\(.*\)\(OpenSCAD-.*-Installer.exe\)\(.*$\)/\\1$file2\\3$file2\\5/" $indexfile
  elif [ $OS == LINUX ]; then
    file2=$2
    sed -i .backup -e "s/^\(.*linux-snapshot-32.*\)\(openscad-.*-32\.tar\.gz\)\(.*\)\(openscad-.*-32\.tar\.gz\)\(.*$\)/\\1$file1\\3$file1\\5/" $indexfile
    sed -i .backup -e "s/^\(.*linux-snapshot-64.*\)\(openscad-.*-64\.tar\.gz\)\(.*\)\(openscad-.*-64\.tar\.gz\)\(.*$\)/\\1$file2\\3$file2\\5/" $indexfile
  fi
  echo "Web page updated. Remember to commit and push openscad.github.com"
else
  echo "Web page not found at $indexfile"
fi
