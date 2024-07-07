#include "rosalia/semver.h"

#include "mirabel/network_adapter.h"

#include "network/adapters/offline_client.h"

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
    //TODO
    return false;
}

static void destroy(network_adapter* self)
{
    //TODO
}

const network_adapter_methods offline_client_app_interface = (network_adapter_methods){
    .name = "offline",
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
