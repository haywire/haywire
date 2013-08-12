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
#### Setup 1
- 1 Azure Large VM (Ubuntu) 4 Haywire instances of Haywire (1 per CPU core) load balanced by HAProxy
- 1 Azure Large VM (Ubuntu) client running Wrk HTTP benchmarking tool.

#### Results
- 601,077 requests/second.
- Average over 800 mbps (Azure network delivers 800 mbps so we are saturating capacity).
- Average less than 40% CPU usage.

![Setup 1 results](http://i.imgur.com/nfFXXpk.png)

#### Setup 2
- 1 Azure Large VM (Ubuntu) 1 Haywire instance with 4 thread (1 per core) event loop fan out.
- 1 Azure Large VM (Ubuntu) client running Wrk HTTP benchmarking tool.

#### Results
- 574,462 requests/second.
- Average 800 mbps (Azure network delivers 800 mbps so we are saturating capacity).
- Average less than 60% CPU usage.
 
###### Latency distribution

        wrk -d10 -t24 -c24 --pipeline 512 --latency http://server:8000
        Running 10s test @ http://server:8000
          24 threads and 24 connections
          Thread Stats   Avg      Stdev     Max   +/- Stdev
            Latency    23.94ms   15.11ms 134.66ms   84.97%
            Req/Sec    25.57k     9.61k   58.60k    65.55%
          Latency Distribution
             50%   19.34ms
             75%   28.39ms
             90%   41.16ms
             99%   79.15ms
          5743304 requests in 10.00s, 0.87GB read
        Requests/sec: 574,462.58
        Transfer/sec:     89.30MB

![Setup 2 results](http://i.imgur.com/nfaz2rB.png)
