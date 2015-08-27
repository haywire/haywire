#!/bin/bash

# Getting libuv
if [ ! -d "lib/libuv" ]; then
    echo "git clone https://github.com/libuv/libuv.git lib/libuv"
    git clone https://github.com/libuv/libuv.git lib/libuv
    cd lib/libuv
    sh autogen.sh
    ./configure
    make
    cd ../../
fi

# Getting Wrk
if [ ! -d "bin/wrk" ]; then
    echo "git clone https://github.com/wg/wrk.git bin/wrk"
    git clone https://github.com/wg/wrk.git bin/wrk
    cd bin/wrk
    make
    cd ../../
fi

# Getting Wrk2
if [ ! -d "bin/wrk2" ]; then
    echo "git clone https://github.com/giltene/wrk2.git bin/wrk2"
    git clone https://github.com/giltene/wrk2.git bin/wrk2
    cd bin/wrk2
    make
    cd ../../
fi
