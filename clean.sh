#!/bin/bash

# Delete build output.
rm -rf builds

# Delete gyp related files.
rm -rf gyp-mac-tool
rm -rf bin/gyp

# Delete make related files.
rm -rf Makefile
rm -rf haywire.Makefile
rm -rf haywire.target.mk
rm -rf hello_world.target.mk

# Delete Visual Studio related files.
rm -rf _ReSharper.haywire
rm -rf haywire.sln
rm -rf haywire.sdf
rm -rf haywire.v11.suo
rm -rf haywire.vcxproj
rm -rf haywire.vcxproj.filters
rm -rf haywire.vcxproj.user
rm -rf hello_world.vcxproj
rm -rf hello_world.vcxproj.filters

# Delete Xcode related files.
rm -rf haywire.xcodeproj

# Delete submodules
rm -rf bin/wrk
rm -rf lib/libuv