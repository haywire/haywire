## Benchmarking with wrk

    ./bin/wrk/wrk -d10 -t8 -c1024 --pipeline 64 --latency http://127.0.0.1:8000
    ./bin/wrk/wrk -d10 -t8 -c8 --pipeline 1024 --latency http://127.0.0.1:8000

## Benchmarking with httperf

    wget https://httperf.googlecode.com/files/httperf-0.9.0.tar.gz

Extract

    ./configure; make; make install
    httperf --server=localhost --port=8000 --num-conns=8 --num-calls=10000

## Benchmarking with Apache Bench

    ab -n 100000 -c 32 -k http://127.0.0.1:8000/
