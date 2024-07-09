#include <cstdlib>
#include <thread>
#include <vector>

#include "rosalia/semver.h"

#include "mirabel/network_adapter.h"

#include "network/adapters/offline_server.h"

/////
// internal

namespace {

    //TODO currently uses an additional inq per connection, which would be unnecessary when in event handling on the server doest *really* have to be done all in the adapter

    //TODO rework tx and rx loop once server side connection to adapter mapping worked out

    struct adapter_context {
        struct connection {
            uint32_t client_id;
            event_queue* outq; // points to inq in offline client
            event_queue* inq; // our owned inq where this client pushes to

            connection()
            {
                inq = (event_queue*)malloc(sizeof(event_queue));
                event_queue_create(inq);
            }

            ~connection()
            {
                event_queue_destroy(inq);
                free(inq);
            }
        };

        std::thread worker;
        event_queue accept;
        std::vector<connection> conns;

        adapter_context()
        {
            event_queue_create(&accept);
        }

        ~adapter_context()
        {
            event_queue_destroy(&accept);
        }
    };

    void adapter_worker(adapter_context* ctx, network_adapter* self)
    {
        // we expect low volume on the offline adapters, so just one thread does server accepting + sending + receiving

        bool exit;

        // accepting
        exit = false;
        while (!exit) {
            event_any e;
            event_queue_pop(&ctx->accept, &e, 0);
            switch (e.base.type) {
                case EVENT_TYPE_NULL: {
                    // pass and loop
                } break;
                case EVENT_TYPE_EXIT: {
                    exit = true;
                    break;
                } break;
                case EVENT_TYPE_LOG: {
                    mirabel_slogf(e.log.status, "offline neta server: queue log: %s", e.log.str);
                } break;
                default: {
                    //TODO handle adapter event for connection accepting
                } break;
            }
            event_destroy(&e);
        }

        // sending
        exit = false;
        while (!exit) {
            event_any e;
            event_queue_pop(&self->outbox, &e, 20); //TODO remove timeout, but for now dont spin so much, because this is one server and event queues dont have a multi poll at the moment
            switch (e.base.type) {
                case EVENT_TYPE_NULL: {
                    // pass and loop
                } break;
                case EVENT_TYPE_EXIT: {
                    exit = true;
                    break;
                } break;
                case EVENT_TYPE_LOG: {
                    mirabel_slogf(e.log.status, "offline neta server: queue log: %s", e.log.str);
                } break;
                //TODO handle adapter event for disconnecting this client
                default: {
                    bool found_and_sent = false;
                    for (size_t conn_idx = 0; conn_idx < ctx->conns.size(); conn_idx++) {
                        if (conn_idx == e.base.client_id) {
                            event_queue_push(ctx->conns[conn_idx].outq, &e);
                            found_and_sent = true;
                            break;
                        }
                    }
                    if (!found_and_sent) {
                        mirabel_slogf(LOGS_WARN, "offline neta server: no client id %u to send event to, dropping", e.base.client_id);
                    }
                } break;
            }
            event_destroy(&e);
        }

        // receiving
        for (size_t conn_id = 0; conn_id < ctx->conns.size(); conn_id++) {
            exit = false;
            while (!exit) {
                event_any e;
                event_queue_pop(ctx->conns[conn_id].inq, &e, 0);
                switch (e.base.type) {
                    case EVENT_TYPE_NULL: {
                        // pass and loop
                    } break;
                    case EVENT_TYPE_EXIT: {
                        exit = true;
                        break;
                    } break;
                    case EVENT_TYPE_LOG: {
                        mirabel_slogf(e.log.status, "offline neta server: client %u inq log: %s", ctx->conns[conn_id].client_id, e.log.str);
                    } break;
                    //TODO handle adapter event for disconnecting client
                    default: {
                        uint32_t true_client_id = ctx->conns[conn_id].client_id;
                        if (e.base.client_id != true_client_id) {
                            mirabel_slogf(LOGS_WARN, "offline neta server: client %u inq with wrong client id %u", true_client_id, e.base.client_id);
                            e.base.client_id = true_client_id;
                        }
                        event_queue_push(self->inbox, &e);
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

static const char* get_last_error(network_adapter* self)
{
    return NULL;
}

static bool create(network_adapter* self)
{
    adapter_context* ctx = new adapter_context();
    ctx->worker = std::thread(adapter_worker, ctx, self);
    self->data = ctx;
    return false;
}

static void destroy(network_adapter* self)
{
    adapter_context* ctx = (adapter_context*)self->data;
    ctx->worker.join();
    delete ctx;
}

const network_adapter_methods offline_server_network_adapter_methods = (network_adapter_methods){
    .name = "offline_server",
    .version = (semver){
        .major = 0,
        .minor = 0,
        .patch = 0,
    },
    .get_last_error = get_last_error,
    .create = create,
    .destroy = destroy,
};

#ifdef __cplusplus
}
#endif
