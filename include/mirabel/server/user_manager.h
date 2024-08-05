#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "rosalia/vector.h"

#include "mirabel/event.h"

#define SERVER_USER_USERNAME_SIZE (32)
#define SERVER_USER_PASSWORD_HASH_SIZE (16)

static const uint64_t USER_ID_NONE = 0;

extern uint64_t next_user_id;

typedef struct server_user_s {
    bool dirty;
    uint64_t id;
    bool is_guest;
    char username[SERVER_USER_USERNAME_SIZE];
    uint8_t password_hash[SERVER_USER_PASSWORD_HASH_SIZE];
    uint64_t password_salt;
} server_user;

typedef struct server_user_manager_s {
    VECTOR(server_user) loaded_slots;
    //TODO MAP(uint64_t, size_t) slots_by_id;
    //TODO
} server_user_manager;

// returns true on failure
bool server_user_manager_create(server_user_manager* self);

void server_user_manager_destroy(server_user_manager* self);

server_user* server_user_manager_user_get_by_id(server_user_manager* self, uint64_t id);

server_user* server_user_manager_user_get_by_name(server_user_manager* self, const char* username);

// read-only on the event
void server_user_manager_handle_event(server_user_manager* self, event_any* e);

uint64_t server_user_manager_user_add(server_user_manager* self, bool is_guest, const char* username, const char* password, uint64_t password_salt);

void server_user_manager_user_remove(server_user_manager* self, uint64_t id);

//TODO move to mirabel global util
void server_user_manager_password_hash(uint8_t password_hash[SERVER_USER_PASSWORD_HASH_SIZE], const char* password, uint64_t password_salt);
