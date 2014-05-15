#pragma once
#include "uv.h"

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#define container_of(ptr, type, member) ((type*)(((char*)(ptr)) - offsetof(type, member)))


union stream_handle2
{
    uv_pipe_t pipe;
    uv_tcp_t tcp;
};

typedef unsigned char handle_storage_t[sizeof(union stream_handle2)];

struct server_ctx
{
    int index;
    handle_storage_t server_handle;
    unsigned int num_connects;
    uv_async_t async_handle;
    uv_thread_t thread_id;
    uv_sem_t semaphore;
};

struct ipc_client_ctx
{
    uv_connect_t connect_req;
    uv_stream_t* server_handle;
    uv_pipe_t ipc_pipe;
    char scratch[16];
};

struct ipc_server_ctx
{
    handle_storage_t server_handle;
    unsigned int num_connects;
    uv_pipe_t ipc_pipe;
};

void connection_consumer_start(void *arg);
