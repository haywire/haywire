#include <stdlib.h>
#include <stdio.h>
#include "uv.h"
#include "connection_consumer.h"
#include "http_server.h"
#include "http_connection.h"
#include "http_response_cache.h"

void ipc_read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
    int rc;
    struct ipc_client_ctx* ctx;
    uv_loop_t* loop;
    uv_handle_type type;
    uv_pipe_t* ipc_pipe;
    
    ipc_pipe = (uv_pipe_t*)handle;
    ctx = container_of(ipc_pipe, struct ipc_client_ctx, ipc_pipe);
    loop = ipc_pipe->loop;
    
    uv_pipe_pending_count(ipc_pipe);
    type = uv_pipe_pending_type(ipc_pipe);
    
    if (type == UV_TCP)
        rc = uv_tcp_init(loop, (uv_tcp_t*) ctx->server_handle);
    else if (type == UV_NAMED_PIPE)
        rc = uv_pipe_init(loop, (uv_pipe_t*) ctx->server_handle, 0);
    
    rc = uv_accept(handle, ctx->server_handle);
    uv_close((uv_handle_t*) &ctx->ipc_pipe, NULL);
}

void ipc_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    struct ipc_client_ctx* ctx;
    ctx = container_of(handle, struct ipc_client_ctx, ipc_pipe);
    buf->base = ctx->scratch;
    buf->len = sizeof(ctx->scratch);
}

void ipc_connect_cb(uv_connect_t* req, int status)
{
    int rc;
    struct ipc_client_ctx* ctx;
    ctx = container_of(req, struct ipc_client_ctx, connect_req);
    rc = uv_read_start((uv_stream_t*)&ctx->ipc_pipe, ipc_alloc_cb, ipc_read_cb);
}

void connection_consumer_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    static char slab[32];
    buf->base = slab;
    buf->len = sizeof(slab);
}

void connection_consumer_new_connection(uv_stream_t* server_handle, int status)
{
    int rc = 0;
    http_connection* connection = create_http_connection();
    http_parser_init(&connection->parser, HTTP_REQUEST);
    
    connection->parser.data = connection;
    connection->stream.data = connection;
    
    rc = uv_tcp_init(server_handle->loop, &connection->stream);
    rc = uv_accept(server_handle, (uv_stream_t*)&connection->stream);
    rc = uv_read_start((uv_stream_t*)&connection->stream, http_stream_on_alloc, http_stream_on_read);
}

void connection_consumer_close(uv_async_t* handle, int status)
{
    struct server_ctx* ctx;
    ctx = container_of(handle, struct server_ctx, async_handle);
    uv_close((uv_handle_t*) &ctx->server_handle, NULL);
    uv_close((uv_handle_t*) &ctx->async_handle, NULL);
}

void get_listen_handle(uv_loop_t* loop, uv_stream_t* server_handle)
{
    int rc;
    struct ipc_client_ctx ctx;
    
    ctx.server_handle = server_handle;
    ctx.server_handle->data = "server handle";
    
    rc = uv_pipe_init(loop, &ctx.ipc_pipe, 1);
    uv_pipe_connect(&ctx.connect_req, &ctx.ipc_pipe, "HAYWIRE_CONNECTION_DISPATCH_PIPE_NAME", ipc_connect_cb);
    rc = uv_run(loop, UV_RUN_DEFAULT);
}

void connection_consumer_start(void *arg)
{
    int rc;
    struct server_ctx *ctx;
    uv_loop_t* loop;
    
    ctx = arg;
    loop = uv_loop_new();
    listener_event_loops[ctx->index] = *loop;
    
    http_request_cache_configure_listener(loop, &listener_async_handles[ctx->index]);
    uv_barrier_wait(listeners_created_barrier);
    
    rc = uv_async_init(loop, &ctx->async_handle, connection_consumer_close);
    uv_unref((uv_handle_t*) &ctx->async_handle);
    
    /* Wait until the main thread is ready. */
    uv_sem_wait(&ctx->semaphore);
    get_listen_handle(loop, (uv_stream_t*) &ctx->server_handle);
    uv_sem_post(&ctx->semaphore);
    
    rc = uv_listen((uv_stream_t*)&ctx->server_handle, 128, connection_consumer_new_connection);
    rc = uv_run(loop, UV_RUN_DEFAULT);
    
    uv_loop_delete(loop);
}
