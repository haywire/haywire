#!/bin/bash
OS=`uname -s`
PLATFORM_WINDOWS="Windows_NT"
PLATFORM_LINUX="Linux"
PLATFORM_MACOSX="Darwin"
GYP=./lib/libuv/build/gyp/gyp
CONFIGURATION="debug"

# Parse command line arguments
while getopts ":o:c:" option
do
    case $option in
        o)
          OS=$OPTARG
          ;;
        c)
          CONFIGURATION=$OPTARG
          ;;
        ?)
          echo "invalid option provided"
          exit
          ;;
  esac
done

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

if [ $OS = $PLATFORM_WINDOWS ]; then
    echo "----------------------------------------"
    echo "Configuring for ${OS} & Visual Studio"
    echo "----------------------------------------"
    $GYP --depth=. -Icommon.gypi -Dlibrary=static_library -Dtarget_arch=x64 --build=$CONFIGURATION haywire.gyp
    msbuild /p:Configuration=$CONFIGURATION haywire.sln
else
    # Compiling wrk
    echo "----------------------------------------"
    echo "Compiling wrk"
    echo "----------------------------------------"
    cd bin/wrk
    make
    cd ../../

    echo "----------------------------------------"
    echo "Configuring and compiling for ${OS}"
    echo "----------------------------------------"
    $GYP --depth=. -Goutput_dir=./builds/unix -Icommon.gypi -Dlibrary=static_library --build=$CONFIGURATION -f make haywire.gyp
fi