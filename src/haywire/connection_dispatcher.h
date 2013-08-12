#pragma once
#include "uv.h"
#include "connection_consumer.h"

struct ipc_peer_ctx
{
    handle_storage_t peer_handle;
    uv_write_t write_req;
};

void start_connection_dispatching(uv_handle_type type, unsigned int num_servers, struct server_ctx* servers, char* listen_address, int listen_port);
