[![Build Status](https://travis-ci.org/kellabyte/Haywire.png?branch=master)](https://travis-ci.org/kellabyte/Haywire)
Haywire
=======
Haywire is an asynchronous HTTP server framework written in C. The goal of Haywire is to learn how to create a server with a minimal feature set that can handle a high rate of requests and connections with as low of latency and resource usage as possible.

Haywire uses the event loop based libuv platform layer that node.js is built on top of (also written in C). libuv abstracts IOCP on Windows and epoll/kqueue/event ports/etc. on Unix systems to provide efficient asynchronous I/O on all supported platforms.

Haywire isn't very useful yet but I wanted to open source it from the very beginning. I started experimenting with a HTTP server for Dazzle queries and I decided I would split it into it's own project.

## Features
- Cross platform (Windows, Linux, OSX)
- HTTP keep-alive
- HTTP pipelining
- Non-blocking I/O

## Plans or Ideas
- HTTP handler routing
- SPDY support
- Export an API that is easily consumable by other languages like on the JVM (Java, etc) and the CLR (C#, etc)

## Contributions
Feel free to join in if you feel like helping progress Haywire. I'm open to new ideas and would love to work with some people instead of by myself!

## Building Haywire
To compile Haywire you need `git` and `python` installed and in your path.

Haywire uses `gyp` which supports generating make type builds or Visual Studio and Xcode projects. The Visual Studio and Xcode projects aren't fully complete so they may not function just yet but they will real soon, hang in there.
    
    git clone https://github.com/kellabyte/Haywire.git

### Compiling on Linux and Mac OSX
    ./build.sh

### Compiling on Windows
Open the Developer Command Prompt for Visual Studio

    build.bat
    
### Current benchmark measurements
#### Setup
- 1 Azure Large VM (Ubuntu) Haywire server running 4 instances of Haywire (1 per CPU core) load balanced by HAProxy
- 1 Azure Large VM (Ubuntu) client running Wrk HTTP benchmarking tool.

#### Results
- 601,077 requests/second
- Average over 800Mbps (Azure network delivers 800Mbps so we are saturating capacity)
- Average less than 40% CPU usage

![Azure benchmarks](http://i.imgur.com/nfFXXpk.png)
