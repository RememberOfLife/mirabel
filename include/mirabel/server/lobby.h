#pragma once

#include <stdbool.h>
#include <stdint.h>

#define SERVER_LOBBY_LOBBYNAME_SIZE (32)
#define SERVER_LOBBY_PASSWORD_HASH_SIZE (16)

static const uint32_t LOBBY_ID_NONE = 0;

typedef struct server_lobby_s {
    bool dirty;
    uint32_t id;
    char lobbyname[SERVER_LOBBY_LOBBYNAME_SIZE];
    uint8_t password_hash[SERVER_LOBBY_PASSWORD_HASH_SIZE];
    uint64_t password_salt;
} server_lobby;

bool server_lobby_create(server_lobby* self);

void server_lobby_destroy(server_lobby* self);
