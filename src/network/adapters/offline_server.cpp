#include <cstdlib>
#include <thread>
#include <vector>

#include "rosalia/semver.h"

#include "mirabel/alloc.h"
#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "mirabel/log.h"
#include "mirabel/network_adapter.h"

#include "network/adapters/offline_server.h"

/////
// internal

namespace {

    //TODO currently uses an additional inq per connection, which would be unnecessary when in event handling on the server doest *really* have to be done all in the adapter

    //TODO rework tx and rx loop once server side connection to adapter mapping worked out

    //TODO remove timeout from queue pops for both client and server, but for now dont spin so much, because this is one server and event queues dont have a multi poll at the moment

    struct adapter_context {
        struct connection {
            uint32_t connection_id;
            event_queue* outq; // points to inq in offline client
            event_queue* inq; // our owned inq where this client pushes to

            bool create(event_queue* client_inq)
            {
                outq = client_inq;
                inq = (event_queue*)mirabel_malloc(sizeof(event_queue));
                event_queue_create(inq);
                return false;
            }

            void destroy() //TODO make sure this is actually used to destroy the queue..
            {
                event_queue_destroy(inq);
                mirabel_free(inq);
            }
        };

        std::thread worker;
        std::vector<connection> conns;
    };

    void adapter_worker(adapter_context* ctx, network_adapter* self)
    {
        bool worker_quit = false;
        while (!worker_quit) {
            // we expect low volume on the offline adapters, so just one thread does server accepting + sending + receiving
            bool exit;

            // sending
            exit = false;
            while (!exit) {
                event_any e;
                event_queue_pop(&self->outbox, &e, 5);
                switch (e.base.type) {
                    case EVENT_TYPE_NULL: {
                        exit = true;
                        break;
                    } break;
                    case EVENT_TYPE_EXIT: {
                        exit = true;
                        break;
                    } break;
                    case EVENT_TYPE_LOG: {
                        mirabel_slogf(e.log.status, "offline neta server: queue log: %s", e.log.str);
                    } break;
                    case EVENT_TYPE_NETWORK_ADAPTER_OFFLINE_CONNECTION_ENTER: {
                        mirabel_slogf(LOGS_LESS, "offline neta server: offline connection enter request received");
                        event_queue* client_rxq = e.neta_offline_conn.rx_queue;
                        adapter_context::connection new_connection{};
                        new_connection.create(client_rxq);
                        ctx->conns.push_back(new_connection);
                        event_any re;
                        event_create_neta_offline_conn_enter(&re, ctx->conns.back().inq);
                        event_queue_push(client_rxq, &re);
                    } break;
                    case EVENT_TYPE_NETWORK_ADAPTER_CLOSE: {
                        mirabel_slogf(LOGS_LESS, "offline neta server: closing adapter");
                        //TODO handle adapter event for disconnecting this client / shutdown?
                        exit = true;
                        worker_quit = true;
                    } break;
                    default: {
                        bool found_and_sent = false;
                        for (size_t conn_idx = 0; conn_idx < ctx->conns.size(); conn_idx++) {
                            if (ctx->conns[conn_idx].connection_id == e.base.connection_id) {
                                event_queue_push(ctx->conns[conn_idx].outq, &e);
                                found_and_sent = true;
                                break;
                            }
                        }
                        if (!found_and_sent) {
                            mirabel_slogf(LOGS_WARN, "offline neta server: no connection id %u to send event to, dropping type %u", e.base.connection_id, e.base.type);
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
                    event_queue_pop(ctx->conns[conn_id].inq, &e, 5);
                    switch (e.base.type) {
                        case EVENT_TYPE_NULL: {
                            exit = true;
                            break;
                        } break;
                        case EVENT_TYPE_EXIT: {
                            exit = true;
                            break;
                        } break;
                        case EVENT_TYPE_LOG: {
                            mirabel_slogf(e.log.status, "offline neta server: client %u inq log: %s", ctx->conns[conn_id].connection_id, e.log.str);
                        } break;
                        case EVENT_TYPE_NETWORK_PROTOCOL_PING: {
                            // response for ping from client
                            event_any re;
                            event_create_type_assoc(&re, EVENT_TYPE_NETWORK_PROTOCOL_PONG, e.base.association_id);
                            event_queue_push(ctx->conns[conn_id].outq, &re);
                        } break;
                        case EVENT_TYPE_NETWORK_ADAPTER_CLOSE: {
                            mirabel_slogf(LOGS_LESS, "offline neta server: received adapter close from client");
                            //TODO client is disconnecting, remove them and send close to client
                            //TODO how to inform server of this somehow, through e.g. session close events
                        } break;
                        default: {
                            uint32_t true_connection_id = ctx->conns[conn_id].connection_id;
                            if (e.base.connection_id != true_connection_id) {
                                mirabel_slogf(LOGS_WARN, "offline neta server: client %u inq with wrong connection id %u", true_connection_id, e.base.workspace_id);
                                e.base.connection_id = true_connection_id;
                            }
                            event_queue_push(self->inbox, &e);
                        } break;
                    }
                    event_destroy(&e);
                }
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
    return NULL;
}

static bool network_adapter_create_mi(network_adapter* self)
{
    adapter_context* ctx = new adapter_context();
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
    delete ctx;
}

const network_adapter_methods offline_server_network_adapter_methods = (network_adapter_methods){
    .name = "offline_server",
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
