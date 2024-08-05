#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "rosalia/vector.h"

#include "mirabel/event.h"

#define SERVER_LOBBY_LOBBYNAME_SIZE (32)
#define SERVER_LOBBY_PASSWORD_HASH_SIZE (16)

static const uint64_t LOBBY_ID_NONE = 0;

typedef struct server_lobby_s {
    bool dirty;
    uint64_t id;
    char lobbyname[SERVER_LOBBY_LOBBYNAME_SIZE];
    uint8_t password_hash[SERVER_LOBBY_PASSWORD_HASH_SIZE];
    uint64_t password_salt;
} server_lobby;

typedef struct server_lobby_manager_s {
    VECTOR(server_lobby) loaded_slots;
    //TODO MAP(uint64_t, size_t) slots_by_id;
    //TODO
} server_lobby_manager;

// returns true on failure
bool server_lobby_manager_create(server_lobby_manager* self);

void server_lobby_manager_destroy(server_lobby_manager* self);

server_lobby* server_lobby_manager_lobby_get_by_id(server_lobby_manager* self, uint64_t id);

server_lobby* server_lobby_manager_lobby_get_by_name(server_lobby_manager* self, const char* lobbyname);

// read-only on the event
void server_lobby_manager_handle_event(server_lobby_manager* self, event_any* e);

uint64_t server_lobby_manager_lobby_add(server_lobby_manager* self, const char* lobbyname, const char* password, uint64_t password_salt);

void server_lobby_manager_lobby_remove(server_lobby_manager* self, uint64_t id);

//TODO move to mirabel global util
void server_lobby_manager_password_hash(uint8_t password_hash[SERVER_LOBBY_PASSWORD_HASH_SIZE], const char* password, uint64_t password_salt);
