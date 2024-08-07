#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "mirabel/server/pwhash.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SERVER_LOBBY_LOBBYNAME_SIZE (32)
#define SERVER_LOBBY_PASSWORD_SIZE (32)

static const uint32_t LOBBY_ID_NONE = 0;

typedef struct server_lobby_s {
    bool dirty;
    uint32_t id;
    char lobbyname[SERVER_LOBBY_LOBBYNAME_SIZE];
    password_hash pwh;
} server_lobby;

bool server_lobby_create(server_lobby* self);

void server_lobby_destroy(server_lobby* self);

#ifdef __cplusplus
}
#endif
