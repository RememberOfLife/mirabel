#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "rosalia/vector.h"

#include "mirabel/server/lobby.h"
#include "mirabel/event.h"

typedef struct server_lobby_manager_s {
    VECTOR(server_lobby) loaded_slots;
    //TODO MAP(uint32_t, size_t) slots_by_id;
    //TODO
} server_lobby_manager;

// returns true on failure
bool server_lobby_manager_create(server_lobby_manager* self);

void server_lobby_manager_destroy(server_lobby_manager* self);

server_lobby* server_lobby_manager_lobby_get_by_id(server_lobby_manager* self, uint32_t id);

server_lobby* server_lobby_manager_lobby_get_by_name(server_lobby_manager* self, const char* lobbyname);

// read-only on the event
void server_lobby_manager_handle_event(server_lobby_manager* self, event_any* e);

uint32_t server_lobby_manager_lobby_add(server_lobby_manager* self, const char* lobbyname, const char* password, uint64_t password_salt);

void server_lobby_manager_lobby_remove(server_lobby_manager* self, uint32_t id);

//TODO move to mirabel global util
void server_lobby_manager_password_hash(uint8_t password_hash[SERVER_LOBBY_PASSWORD_HASH_SIZE], const char* password, uint64_t password_salt);
