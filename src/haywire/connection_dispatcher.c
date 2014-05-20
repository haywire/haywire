#include <stdlib.h>
#include <stdio.h>
#include "uv.h"
#include "connection_dispatcher.h"
#include "connection_consumer.h"

static struct sockaddr_in listen_addr;

void ipc_close_cb(uv_handle_t* handle)
{
    struct ipc_peer_ctx* ctx;
    ctx = container_of(handle, struct ipc_peer_ctx, peer_handle);
    free(ctx);
}

void ipc_write_cb(uv_write_t* req, int status)
{
    struct ipc_peer_ctx* ctx;
    ctx = container_of(req, struct ipc_peer_ctx, write_req);
    uv_close((uv_handle_t*) &ctx->peer_handle, ipc_close_cb);
}

void ipc_connection_cb(uv_stream_t* ipc_pipe, int status)
{
    int rc;
    struct ipc_server_ctx* sc;
    struct ipc_peer_ctx* pc;
    uv_loop_t* loop;
    uv_buf_t buf;
    
    loop = ipc_pipe->loop;
    buf = uv_buf_init("PING", 4);
    sc = container_of(ipc_pipe, struct ipc_server_ctx, ipc_pipe);
    pc = calloc(1, sizeof(*pc));
    //ASSERT(pc != NULL);
    
    if (ipc_pipe->type == UV_TCP)
        rc = uv_tcp_init(loop, (uv_tcp_t*) &pc->peer_handle);
    else if (ipc_pipe->type == UV_NAMED_PIPE)
        rc = uv_pipe_init(loop, (uv_pipe_t*) &pc->peer_handle, 1);
    
    rc = uv_accept(ipc_pipe, (uv_stream_t*) &pc->peer_handle);
    rc = uv_write2(&pc->write_req,
                          (uv_stream_t*) &pc->peer_handle,
                          &buf,
                          1,
                          (uv_stream_t*) &sc->server_handle,
                          ipc_write_cb);
    
    if (--sc->num_connects == 0)
        uv_close((uv_handle_t*) ipc_pipe, NULL);
}

/* Set up an IPC pipe server that hands out listen sockets to the worker
 * threads. It's kind of cumbersome for such a simple operation, maybe we
 * should revive uv_import() and uv_export().
 */
void start_connection_dispatching(uv_handle_type type, unsigned int num_servers, struct server_ctx* servers, char* listen_address, int listen_port)
{
    int rc;
    struct ipc_server_ctx ctx;
    uv_loop_t* loop;
    unsigned int i;
    
    loop = uv_default_loop();
    ctx.num_connects = num_servers;
    
    if (type == UV_TCP)
    {
        uv_ip4_addr(listen_address, listen_port, &listen_addr);
        
        rc = uv_tcp_init(loop, (uv_tcp_t*) &ctx.server_handle);
        rc = uv_tcp_bind((uv_tcp_t*) &ctx.server_handle, (const struct sockaddr*)&listen_addr, 0);
        printf("Listening on %s:%d\n", listen_address, listen_port);
    }
    
    rc = uv_pipe_init(loop, &ctx.ipc_pipe, 1);
    rc = uv_pipe_bind(&ctx.ipc_pipe, "HAYWIRE_CONNECTION_DISPATCH_PIPE_NAME");
    rc = uv_listen((uv_stream_t*) &ctx.ipc_pipe, 128, ipc_connection_cb);
    
    for (i = 0; i < num_servers; i++)
        uv_sem_post(&servers[i].semaphore);
    
    rc = uv_run(loop, UV_RUN_DEFAULT);
    uv_close((uv_handle_t*) &ctx.server_handle, NULL);
    rc = uv_run(loop, UV_RUN_DEFAULT);
    
    for (i = 0; i < num_servers; i++)
        uv_sem_wait(&servers[i].semaphore);
}
