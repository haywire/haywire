#!/bin/bash
../bin/wrk/wrk --script ./get.lua --latency -d 30s -t 8 -c 32 -R 30000 http://127.0.0.1:8000 -- 64
