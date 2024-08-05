#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "rosalia/noise.h"
#include "rosalia/rand.h"
#include "rosalia/timestamp.h"
#include "rosalia/vector.h"

#include "mirabel/application.h"
#include "mirabel/event.h"
#include "mirabel/server.h"

#include "mirabel/server/lobby_manager.h"

//TODO using appi.aserver is ugly here..

uint64_t next_lobby_id = 1;

bool server_lobby_manager_create(server_lobby_manager* self)
{
    //TODO
    return false;
}

void server_lobby_manager_destroy(server_lobby_manager* self)
{
    //TODO
}

server_lobby* server_lobby_manager_lobby_get_by_id(server_lobby_manager* self, uint64_t id)
{
    //TODO
}

server_lobby* server_lobby_manager_lobby_get_by_name(server_lobby_manager* self, const char* lobbyname)
{
    //TODO
}

// read-only on the event
void server_lobby_manager_handle_event(server_lobby_manager* self, event_any* e)
{
    //TODO
}

uint64_t server_lobby_manager_lobby_add(server_lobby_manager* self, const char* lobbyname, const char* password, uint64_t password_salt)
{
    //TODO
}

void server_lobby_manager_lobby_remove(server_lobby_manager* self, uint64_t id)
{
    //TODO
}

void server_lobby_manager_password_hash(uint8_t password_hash[SERVER_LOBBY_PASSWORD_HASH_SIZE], const char* password, uint64_t password_salt)
{
    //TODO
}
