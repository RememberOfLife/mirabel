#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct server_s {
    bool offline;
    // db connection
    // network_adapter* nets;
    // connection* connections;
    // user_info* users;
    // lobby* lobbies;
    // session* sessions;
} server;

bool server_create(server* srv, bool offline);

void server_destroy(server* srv);

bool server_update(server* srv);

#ifdef __cplusplus
}
#endif
