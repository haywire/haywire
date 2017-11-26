[![Join us on #haywire on Freenode](https://kiwiirc.com/buttons/chat.freenode.net/haywire.png)](https://kiwiirc.com/client/chat.freenode.net:+6697/#haywire)
[![Build Status](https://travis-ci.org/haywire/haywire.svg?branch=master)](https://travis-ci.org/haywire/haywire)

Haywire
=======
Haywire is an asynchronous HTTP server framework written in C. The goal of Haywire is to learn how to create a server with a minimal feature set that can handle a high rate of requests and connections with as low of latency and resource usage as possible.

Haywire uses the event loop based [libuv](https://github.com/joyent/libuv) platform layer that node.js is built on top of (also written in C). libuv abstracts IOCP on Windows and epoll/kqueue/event ports/etc. on Unix systems to provide efficient asynchronous I/O on all supported platforms.

Haywire isn't very useful yet but I wanted to open source it from the very beginning. I started experimenting with a HTTP server for Dazzle queries and I decided I would split it into its own project.

## Features
- Cross platform (Windows, Linux, OSX)
- HTTP keep-alive
- HTTP pipelining
- Non-blocking I/O
- SO_REUSEPORT multi-process and multi-threaded load balancing across CPU cores.

## Plans or Ideas
- HTTP handler routing
- SPDY support
- Export an API that is easily consumable by other languages like on the JVM (Java, etc) and the CLR (C#, etc)

## Contributions
Feel free to join in if you feel like helping progress Haywire. I'm open to new ideas and would love to work with some people instead of by myself!

## Dependencies
```
apt-get install git gcc make cmake automake autoconf libssl-dev libtool
```

## Compiling on Linux
```
mkdir build && cd build && cmake .. && make -j5
```
    
## Compiling on Mac OSX
```
brew install automake
brew install libtool
brew install openssl
mkdir build && cd build && cmake .. && make -j5
```

## Compiling on Windows
These instructions are old but may still work. We need to make Windows support in `make.sh`.

Open the Developer Command Prompt for Visual Studio

    build.bat

## Compiling for Windows on Linux(Ubuntu)
```
sudo apt-get install mingw-w64 g++-mingw-w64
mkdir build && cd build && cmake .. -DCMAKE_TOOLCHAIN_FILE=Toolchain-Ubuntu-mingw64.cmake && make -j5
```
    
## Benchmarks

Bare metal Rackspace instance    
Intel(R) Xeon(R) CPU E5-2680 v2 @ 2.80GHz 20 physical cores

#### Throughput
```
../bin/wrk/wrk --script ./pipelined_get.lua --latency -d 5m -t 40 -c 760 http://server:8000 -- 32

Running 5m test @ http://server:8000
  40 threads and 760 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     2.04ms    5.38ms 389.00ms   92.75%
    Req/Sec   233.38k    48.72k  458.99k    86.19%
  Latency Distribution
     50%    1.26ms
     75%    1.96ms
     90%    4.09ms
     99%    0.00us
  2781077938 requests in 5.00m, 409.23GB read
Requests/sec: 9,267,161.41
Transfer/sec:      1.36GB
```

#### Latency distribution with coordinated omission at 3.5 million requests/second
```
../bin/wrk2/wrk --script ./pipelined_get.lua --latency -d 10s -t 80 -c 512 -R 3500000 http://server:8000 -- 32

Running 10s test @ http://server:8000
  80 threads and 512 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     1.82ms    3.00ms  64.86ms   94.17%
    Req/Sec       -nan      -nan   0.00      0.00%
  Latency Distribution (HdrHistogram - Recorded Latency)
 50.000%    1.18ms
 75.000%    1.62ms
 90.000%    2.34ms
 99.000%   16.75ms
 99.900%   33.12ms
 99.990%   44.70ms
 99.999%   52.99ms
100.000%   64.89ms

----system---- ----total-cpu-usage---- ------memory-usage----- --io/total- -dsk/total- -net/total- ---system-- -pkt/total- ----tcp-sockets----
     time     |usr sys idl wai hiq siq| used  buff  cach  free| read  writ| read  writ| recv  send| int   csw |#recv #send|lis act syn tim clo
03-12 18:58:10|  0   0  99   0   0   0| 712M 95.3M 1203M  124G|0.05  0.22 | 684B   10k|   0     0 |5066  1368 |   0     0 |  3   2   0   0   0
03-12 18:58:11|  0   0 100   0   0   0| 711M 95.3M 1203M  124G|   0     0 |   0     0 |5538B 1588B| 425   975 |92.0  2.00 |  3   2   0   0   0
03-12 18:58:12|  5   2  92   0   0   1| 725M 95.3M 1203M  124G|   0     0 |   0     0 |  85M  285M| 220k  112k| 127k  226k|  3 482   0   0   1
03-12 18:58:13| 10   4  85   0   0   1| 724M 95.3M 1203M  124G|   0  2.00 |   0    32k| 166M  556M| 427k  173k| 243k  438k|  3 482   0   0   1
03-12 18:58:14| 10   4  85   0   0   1| 725M 95.3M 1203M  124G|   0     0 |   0     0 | 165M  555M| 435k  172k| 243k  438k|  3 482   0   0   1
03-12 18:58:15| 10   4  85   0   0   1| 723M 95.3M 1203M  124G|   0     0 |   0     0 | 166M  555M| 440k  172k| 243k  438k|  3 482   0   0   1
03-12 18:58:16| 10   4  86   0   0   1| 724M 95.3M 1203M  124G|   0     0 |   0     0 | 166M  555M| 415k  172k| 243k  438k|  3 482   0   0   1
03-12 18:58:17| 10   4  85   0   0   1| 723M 95.3M 1203M  124G|   0     0 |   0     0 | 165M  555M| 404k  172k| 243k  438k|  3 482   0   0   1
03-12 18:58:18| 10   4  85   0   0   1| 724M 95.3M 1203M  124G|   0  5.00 |   0    24k| 165M  555M| 404k  171k| 243k  438k|  3 482   0   0   1
03-12 18:58:19| 10   4  85   0   0   1| 724M 95.3M 1203M  124G|   0     0 |   0     0 | 166M  555M| 411k  171k| 243k  438k|  3 482   0   0   1
03-12 18:58:20| 10   4  85   0   0   1| 723M 95.3M 1203M  124G|   0     0 |   0     0 | 166M  555M| 412k  170k| 244k  438k|  3 482   0   0   1
03-12 18:58:21| 10   4  85   0   0   1| 722M 95.3M 1203M  124G|   0     0 |   0     0 | 166M  555M| 411k  171k| 244k  438k|  3 482   0   0   1
03-12 18:58:22|  5   2  93   0   0   1| 724M 95.3M 1203M  124G|   0     0 |   0     0 |  76M  256M| 190k   81k| 113k  202k|  3   2   0   0 391
```

## Users of Haywire
There's a production system running Haywire that serves `700 million requests a day` from an Aerospike cluster with `2ms or lower` response times.

[pyrs](https://github.com/skogorev/pyrs) Python handlers for Haywire.
