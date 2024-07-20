#include <cstdlib>
#include <thread>

#include "rosalia/semver.h"

#include "mirabel/application.h"
#include "mirabel/network_adapter.h"
#include "mirabel/server.h"

#include "network/adapters/offline_client.h"

/////
// internal

namespace {

    struct adapter_context {
        std::thread worker;
        uint32_t connection_id;
        event_queue* outq = NULL;
        event_queue inq;
    };

    void adapter_worker(adapter_context* ctx, network_adapter* self)
    {
        bool worker_quit = false;
        while (!worker_quit) {
            // we expect low volume on the offline adapters, so just one thread does client sending + receiving

            bool exit;

            // sending
            // get from self->outbox and push to ctx->outq
            exit = false;
            while (!exit) {
                event_any e;
                event_queue_pop(&self->outbox, &e, UINT32_MAX);
                switch (e.base.type) {
                    case EVENT_TYPE_NULL: {
                        // pass and loop
                    } break;
                    case EVENT_TYPE_EXIT: {
                        exit = true;
                        break;
                    } break;
                    case EVENT_TYPE_LOG: {
                        mirabel_slogf(e.log.status, "offline neta client: queue log: %s", e.log.str);
                    } break;
                    case EVENT_TYPE_NETWORK_ADAPTER_OPEN: {
                        event_queue* srv_inq = appi.aserver->netas[0]->inbox; //TODO //HACK better way of finding the server offline connector //BUG this is a race condition!
                        event_any re;
                        event_create_neta_offline_conn_enter(&re, &ctx->inq);
                        event_queue_push(srv_inq, &re);
                    } break;
                    case EVENT_TYPE_NETWORK_ADAPTER_CLOSE: {
                        event_any re;
                        event_create_neta_close(&re, NULL);
                        event_queue_push(ctx->outq, &re);
                        ctx->outq = NULL;
                    } break;
                    default: {
                        if (ctx->outq == NULL) {
                            mirabel_slogf(LOGS_WARN, "offline neta client: failed to send event, missing outq");
                        } else {
                            event_queue_push(ctx->outq, &e);
                        }
                    } break;
                }
                event_destroy(&e);
            }

            // receiving
            // get from ctx->inq and push to self->inbox
            exit = false;
            while (!exit) {
                event_any e;
                event_queue_pop(&ctx->inq, &e, UINT32_MAX);
                switch (e.base.type) {
                    case EVENT_TYPE_NULL: {
                        // pass and loop
                    } break;
                    case EVENT_TYPE_EXIT: {
                        exit = true;
                        break;
                    } break;
                    case EVENT_TYPE_LOG: {
                        mirabel_slogf(e.log.status, "offline neta client: queue log: %s", e.log.str);
                    } break;
                    case EVENT_TYPE_NETWORK_ADAPTER_OFFLINE_CONNECTION_ENTER: {
                        ctx->outq = e.neta_offline_conn.rx_queue;
                        event_any re;
                        event_create_type(&re, EVENT_TYPE_NETWORK_ADAPTER_OPEN);
                        event_queue_push(self->inbox, &re);
                        event_create_neta_veri(&re, EVENT_TYPE_NETWORK_ADAPTER_VERIFICATION_ACCEPT, BLOB_NULL, NULL);
                        event_queue_push(self->inbox, &re);
                    } break;
                    case EVENT_TYPE_NETWORK_ADAPTER_CLOSE: {
                        if (ctx->outq == NULL) {
                            // client initiated disconnect
                            event_any re;
                            event_create_neta_close(&re, NULL);
                            event_queue_push(self->inbox, &re);
                            exit = true;
                            worker_quit = true;
                        } else {
                            //TODO alternatively, maybe the server has initiated the drop, then do ??
                            // same thing actually..
                        }
                    } break;
                    default: {
                        if (e.base.connection_id != ctx->connection_id) {
                            mirabel_slogf(LOGS_WARN, "offline neta client: connection id %u received event with wrong connection id %u, dropping", ctx->connection_id, e.base.connection_id);
                        } else {
                            event_queue_push(self->inbox, &e);
                        }
                    } break;
                }
                event_destroy(&e);
            }
        }
    }

} // namespace

/////
// methods wrapper

#ifdef __cplusplus
extern "C" {
#endif

static const char* network_adapter_get_last_error_mi(network_adapter* self)
{
    //TODO
    return NULL;
}

static bool network_adapter_create_mi(network_adapter* self)
{
    adapter_context* ctx = (adapter_context*)malloc(sizeof(adapter_context));
    ctx->worker = std::thread(adapter_worker, ctx, self);
    self->data = ctx;
    return false;
}

static void network_adapter_destroy_mi(network_adapter* self)
{
    event_any e;
    event_create_neta_close(&e, NULL);
    event_queue_push(&self->outbox, &e);
    adapter_context* ctx = (adapter_context*)self->data;
    ctx->worker.join();
    free(self->data);
}

const network_adapter_methods offline_client_network_adapter_methods = (network_adapter_methods){
    .name = "offline_client",
    .version = (semver){
        .major = 0,
        .minor = 0,
        .patch = 0,
    },
    .get_last_error = network_adapter_get_last_error_mi,
    .create = network_adapter_create_mi,
    .destroy = network_adapter_destroy_mi,
};

#ifdef __cplusplus
}
#endif
