#!/usr/bin/env bash

# change to the install source directory
cd "$( dirname "$( type -p $0 )" )"

if ! [ -f bin/openscad -a -d lib/openscad -a -d examples -a -d libraries ]; then
	echo "Error: Can't change to install source directory!" >&2
	exit 1
fi

echo "This will install openscad. Please enter the install prefix"
echo "or press Ctrl-C to abort the install process:"
read -p "[/usr/local]: " prefix

if [ "$prefix" = "" ]; then
	prefix="/usr/local"
fi

if [ ! -d "$prefix" ]; then
	echo; echo "Install prefix \`$prefix' does not exist. Press ENTER to continue"
	echo "or press Ctrl-C to abort the install process:"
	read -p "press enter to continue> "
fi

mkdir -p "$prefix"/{bin,lib/openscad,share/openscad/examples,share/openscad/libraries}

if ! [ -w "$prefix"/bin/ -a -w "$prefix"/lib/openscad -a -w "$prefix"/share/openscad ]; then
	echo "You does not seam to have write permissions for prefix \`$prefix'!" >&2
	echo "Maybe you should have run this install script using \`sudo'?" >&2
	exit 1
fi

echo "Copying application wrappers..."
cp -rv bin/. "$prefix"/bin/

echo "Copying application..."
cp -rv lib/. "$prefix"/lib/

echo "Copying examples..."
cp -rv examples/. "$prefix"/share/openscad/examples/

echo "Copying libraries..."
cp -rv libraries/. "$prefix"/share/openscad/libraries/

echo "Copying support files..."
cp -rv share/. "$prefix"/share/

echo "Installation finished. Have a nice day."
