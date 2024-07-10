#include <cstdlib>
#include <thread>

#include "rosalia/semver.h"

#include "mirabel/network_adapter.h"

#include "network/adapters/offline_client.h"

/////
// internal

namespace {

    struct adapter_context {
        std::thread worker;
        uint32_t client_id;
        event_queue* outq;
        event_queue inq;
    };

    void adapter_worker(adapter_context* ctx, network_adapter* self)
    {
        bool worker_quit = false;
        while (!worker_quit) {
            // we expect low volume on the offline adapters, so just one thread does client sending + receiving

            bool exit;

            // sending
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
                    //TODO handle adapter events for connecting and disconnecting
                    default: {
                        event_queue_push(ctx->outq, &e);
                    } break;
                }
                event_destroy(&e);
            }

            // receiving
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
                    //TODO handle adapter event for disconnected from server
                    default: {
                        if (e.base.session_id != ctx->client_id) {
                            mirabel_slogf(LOGS_WARN, "offline neta client: client id %u received event with wrong client id %u, dropping", ctx->client_id, e.base.session_id);
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

static const char* get_last_error(network_adapter* self)
{
    //TODO
    return NULL;
}

static bool create(network_adapter* self)
{
    adapter_context* ctx = (adapter_context*)malloc(sizeof(adapter_context));
    ctx->worker = std::thread(adapter_worker, ctx, self);
    self->data = ctx;
    return false;
}

static void destroy(network_adapter* self)
{
    //TODO send adapter event for shutdown to outbox
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
    .get_last_error = get_last_error,
    .create = create,
    .destroy = destroy,
};

#ifdef __cplusplus
}
#endif
