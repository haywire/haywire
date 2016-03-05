# Buffer management

Haywire allocates buffers dynamically, allowing them to be extended as required to accommodate the data being read by libuv. Buffers are limited to 1MB by default to avoid having the caller exhaust the available memory on the server.

## Buffer chunks

By default, Haywire allocates a new buffer that is 64KB long. libuv doesn't have access to the entire underlying buffer space, as buffer management is abstracted away to allow buffer re-use.

Initially, the buffer is seen as a big 64KB chunk that is handed over to libuv so it can read data into it. However, since libuv may use only a fraction of that buffer, subsequent buffer requests will most likely be served by returning free space within the current underlying buffer.

Whenever libuv asks for a new buffer, Haywire returns the remaining free space in the buffer as a chunk of memory that libuv can read data into.

Example:
1) libuv requests a 64KB chunk
2) Haywire allocated a 64KB buffer and returns a buffer chunk that overlaps the entire buffer and is 64KB long
3) libuv reads 10k bytes into that chunk of memory and passes it onto to Haywire for parsing
4) libuv requests another 64KB chunk to read more incoming data
5) Haywire sees that the buffer still has 54k bytes free, it returns that free space as a 54KB chunk, starting at a 10KB offset since the beginning of the underlying buffer.


libuv and Haywire keep using the same buffer until the buffer reaches a sufficiently high level of usage (currently 50%), at which time Haywire reallocates the underlying buffer (see [Dynamic reallocation](#dynamic-reallocation)). In practice, unless requests are pipelined and come in batches big enough to use more than 50% of the buffer or unless the request is big enough to use more than 50% of the buffer, reallocation doesn't really happen, as Haywire continuously sweeps old data away from the buffer (see [Buffer housekeeping](#buffer-housekeeping)).

## Dynamic reallocation

Since buffers are extended by calling `realloc`, it's possible that the buffer will be moved around as the request is being processed. Therefore, Haywire places pins on certain pointers (e.g. URL start, header start, body start) as it parses requests in the buffer. When a new pin is created, Haywire calculates the pointer offset on the request buffer (i.e. difference between the pointer value and the request buffer start pointer) and that offset can then be used to locate the original pointer (e.g. URL start) even when the buffer is relocated by `realloc`. Once the request is fully parsed, Haywire locates all the pins and recalculates pointers so that none of them are dangling in the event of a buffer relocation.

Example:

```

1) A 512KB request is partially read into a 64KB buffer

+--------------------------------------------------------------------------------------+
| POST / HTTP/1.1\r\nHost: someserver.com\r\nLorem ipsum dolor sit amet, .........     |
+--------------------------------------------------------------------------------------+

2) Haywire finds the URL position 5, the Host header at position 17, the header value at position 23 and
the start of the body at position 39. Therefore, it places the following pins:

    url pin => key = request->url pointer; offset = 5
    host header name pin => key = host header name pointer; offset = 17
    host header value pin => key = host header value pointer; offset = 23
    body pin => key = request->body pointer; offset = 39

3) libuv requests another chunk. Since the buffer is full, it it gets resized with realloc
and (in this example) no longer starts at the same location it did previously. Therefore,
the URL, host value/key and body pointers are now invalid.

4) libuv and Haywire keep processing the request and eventually the request is completely parsed.
Haywire then locates all the pins and recalculates the pointers.

   Example (pseudo-code):
      url_pointer := locate(key = URL pointer)

      locate(key) = buffer start + offset(key)

5) The request callback can now safely execute as the request is stored in the buffer just as
if it had been read in one go.

```


## Buffer housekeeping

To keep the memory footprint low, Haywire uses a mark/sweep approach to discard the raw data for requests that have been fully parsed and executed. The following diagrams shows an example of how Haywire handles cleans up the buffer as it processes requests.


```
1) The client starts sending requests and libuv requests a new buffer,
hinting at a size of 64KB by default.

2) Haywire allocates a new buffer that is 64KB long.
Initially, the buffer is seen as a big 64KB chunk, but as libuv consumes part of it,
subsequent memory requests are served from within the bufferSince libuv may not
use all of the 64KB and returns that a single 64KB chunk back to libuv.

+ buffer
|
|
+-> +--------------------------------------------------------------------------------------+
    |                                                                                      |
+-> +--------------------------------------------------------------------------------------+
|
|
+ chunk


3) libuv reads the first request (e.g. GET /) in one go

+ buffer
|
|
+-> +--------------------------------------------------------------------------------------+
    | GET / HTTP/1.1\r\n\r\n |                                                             |
+-> +--------------------------------------------------------------------------------------+
|
|
+ chunk                                     buffer->used = 18


4) Haywire parses the request. When the end of the request if found,
Haywire places a mark at the end of the previous chunk. However,
since this is the previous chunk, the mark is set at the beginning of the buffer.

+ buffer
|
| + mark
| |
| |
+-> +--------------------------------------------------------------------------------------+
    | GET / HTTP/1.1\r\n\r\n |                                                             |
+-> +--------------------------------------------------------------------------------------+
|
|
+ chunk                                     buffer->used = 18


5) Haywire reaches the end of the chunk soon after and tries to sweep the buffer.
Since the mark is right at the beginning of the buffer, nothing really happens.

6) libuv asks for another chunk and reads 27 bytes.

+ buffer
|
| + mark
| |
| |
+-> +--------------------------------------------------------------------------------------+
    | GET / HTTP/1.1\r\n\r\n | GET HTTP/1.1\r\n\r\nGET / HTT |                             |
    +--------------------------------------------------------------------------------------+
                             ^
                             |
                             |
chunk  +---------------------+                buffer->used = 45
                                              buffer->last_used = 18


7) Haywire parses the second chunk and eventually it finds the end of the second request. Once
that happens, a mark is placed at the end of the previous chunk (i.e. by advancing the underlying
buffer by buffer->last_used bytes)

+ buffer
|
|                            + mark
|                            |
|                            v
+-> +--------------------------------------------------------------------------------------+
    | GET / HTTP/1.1\r\n\r\n | GET HTTP/1.1\r\n\r\nGET / HTT |                             |
    +--------------------------------------------------------------------------------------+
                             ^
                             |
                             |
 chunk +---------------------+                buffer->used = 45
                                              buffer->last_used = 18

8) Haywire reaches the end of the buffer and the third request is now in flight.
Haywire sweeps everything before the mark, by copying the remainder of the data in the buffer
back to the beginning. Any pins placed while parsing the request being discarded (i.e. first URL)
are removed and any other pins (e.g. URL of the last request) are adjusted by the number of bytes
being swept.

+ buffer
|
| + mark
| |
| |
+-> +--------------------------------------------------------------------------------------+
    | GET HTTP/1.1\r\n\r\nGET / HTT |                                                      |
    +--------------------------------------------------------------------------------------+

                                              buffer->used = 27
                                              buffer->last_used = 0

9) libuv asks for another buffer and Haywire returns a new chunk that overlaps with the
free space in the underlying buffer

+ buffer
|
| + mark
| |
| |
+-> +--------------------------------------------------------------------------------------+
    | GET HTTP/1.1\r\n\r\nGET / HTT |                                                      |
    +--------------------------------------------------------------------------------------+
                                    ^
                                    |
   chunk +--------------------------+
                                              buffer->used = 27
                                              buffer->last_used = 0


10) libuv reads the rest of the request into the new chunk. Haywire parses the chunk and the process
repeats.

+ buffer
|
| + mark
| |
| |
+-> +--------------------------------------------------------------------------------------+
    | GET HTTP/1.1\r\n\r\nGET / HTT | P / 1.1\r\n\r\n                                      |
    +--------------------------------------------------------------------------------------+
                                    ^
                                    |
   chunk +--------------------------+
                                              buffer->used = 36
                                              buffer->last_used = 27

```


