#!/bin/bash
./compile_dependencies.sh

rm -rf build
mkdir build
cd build
cmake -DCMAKE_OSX_ARCHITECTURES=x86_64 -G Xcode ..
xcodebuild -project haywire.xcodeproj/
