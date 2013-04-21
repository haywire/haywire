#!/bin/bash
OS=`uname -s`
PLATFORM_WINDOWS="Windows_NT"
PLATFORM_LINUX="Linux"
PLATFORM_MACOSX="Darwin"
PLATFORM_UNIX=false
GYP=./lib/libuv/build/gyp/gyp

if [ $OS != "Windows_NT" ]; then
    PLATFORM_UNIX=true
fi

echo "----------------------------------------"
echo "Cloning submodules"
echo "----------------------------------------"
git submodule update --init --recursive

# Getting Gyp build environment.
if [ ! -d "bin/gyp" ]; then
    echo "git clone https://git.chromium.org/external/gyp.git bin/gyp"
    git clone https://git.chromium.org/external/gyp.git bin/gyp
fi
if [ ! -d "lib/libuv/build" ]; then
    mkdir lib/libuv/build
    cp -Rf bin/gyp lib/libuv/build/gyp
fi

if [ ! $PLATFORM_UNIX ]; then
	echo "----------------------------------------"
	echo "Configuring for ${OS} and Visual Studio"
	echo "----------------------------------------"
	
	$GYP --depth=. -Dlibrary=static_library -Dtarget=ia32 haywire.gyp
else
	# Compiling wrk
	echo "----------------------------------------"
	echo "Compiling wrk"
	echo "----------------------------------------"
	cd bin/wrk
	make
	cd ../../

	echo "----------------------------------------"
	echo "Configuring for ${OS}"
	echo "----------------------------------------"
	$GYP -f make --depth=. -Dlibrary=static_library haywire.gyp

	echo "----------------------------------------"
	echo "Compiling Haywire"
	echo "----------------------------------------"
	./clean.sh
	make
fi