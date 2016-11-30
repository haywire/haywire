# Memory allocation benchmarks
Questions we should get clarity on are the following.

1. Which allocator gives us the most throughput (requests/second).
2. Which allocator gives us the lowest latency.
3. Which allocator gives us the lowest memory usage.

## Throughput

#### Malloc
```
../bin/wrk/wrk --script ./pipelined_get.lua --latency -d 60s -t 40 -c 768 http://server:8001 -- 128

VIRT 1,410.0MB  
RES     64.2MB

Running 1m test @ http://server:8001
  40 threads and 768 connections
  
  441603849 requests in 1.00m, 64.57GB read
Requests/sec: 7347393.20
Transfer/sec:      1.07GB
```

#### TCMalloc
```
../bin/wrk/wrk --script ./pipelined_get.lua --latency -d 60s -t 40 -c 768 http://server:8002 -- 128

VIRT 358.0MB  
RES   85.0MB

Running 1m test @ http://server:8002
  40 threads and 768 connections
  
  570599580 requests in 1.00m, 83.43GB read
Requests/sec: 9493994.51
Transfer/sec:      1.39GB
```

#### JEMalloc
```
../bin/wrk/wrk --script ./pipelined_get.lua --latency -d 60s -t 40 -c 768 http://server:8003 -- 128

VIRT 363.0MB  
RES   70.4MB

Running 1m test @ http://server:8003
  40 threads and 768 connections
  
  555576460 requests in 1.00m, 81.24GB read
Requests/sec: 9244075.62
Transfer/sec:      1.35GB
```

#### Lockless
```
../bin/wrk/wrk --script ./pipelined_get.lua --latency -d 60s -t 40 -c 768 http://server:8004 -- 128

VIRT 530.0MB  
RES  166.0MB

Running 1m test @ http://server:8004
  40 threads and 768 connections
  
  567374962 requests in 1.00m, 82.96GB read
Requests/sec: 9440641.47
Transfer/sec:      1.38GB
```

## Latency distribution
The latency distribution benchmarks are using [wrk2](https://github.com/giltene/wrk2) from Gile Tene which does proper latency calculations using HdrHistogram and coordination omission. They run with a `3 million requests/second` constant rate.

![Latency distribution](http://i.imgur.com/Mt1EqEl.png)

## Conclusion
> 1. Which allocator gives us the most throughput (requests/second).

TCMalloc

> 2. Which allocator gives us the lowest latency.

Lockless

> 3. Which allocator gives us the lowest memory usage.

Malloc
