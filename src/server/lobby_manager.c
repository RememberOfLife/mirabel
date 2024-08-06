#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "rosalia/noise.h"
#include "rosalia/rand.h"
#include "rosalia/timestamp.h"
#include "rosalia/vector.h"

#include "mirabel/server/lobby.h"
#include "mirabel/application.h"
#include "mirabel/event.h"
#include "mirabel/server.h"

#include "mirabel/server/lobby_manager.h"

//TODO using appi.aserver is ugly here.., make server* srv the first arg for all functions instead..

uint64_t next_lobby_id = 1;

bool server_lobby_manager_create(server_lobby_manager* self)
{
    VEC_CREATE(&self->loaded_slots, 8);
    return false;
}

void server_lobby_manager_destroy(server_lobby_manager* self)
{
    //TODO lobbies will contain complex data, actually destroy that
    VEC_DESTROY(&self->loaded_slots);
}

server_lobby* server_lobby_manager_lobby_get_by_id(server_lobby_manager* self, uint32_t id)
{
    //TODO use map to make this faster
    for (size_t search_idx = 0; search_idx < VEC_LEN(&self->loaded_slots); search_idx++) {
        if (self->loaded_slots[search_idx].id == id) {
            return &self->loaded_slots[search_idx];
        }
    }
    return NULL;
}

server_lobby* server_lobby_manager_lobby_get_by_name(server_lobby_manager* self, const char* lobbyname)
{
    //TODO use map to make this faster
    for (size_t search_idx = 0; search_idx < VEC_LEN(&self->loaded_slots); search_idx++) {
        if (strcmp(self->loaded_slots[search_idx].lobbyname, lobbyname) == 0) {
            return &self->loaded_slots[search_idx];
        }
    }
    return NULL;
}

// read-only on the event
void server_lobby_manager_handle_event(server_lobby_manager* self, event_any* e)
{
    if (e->base.workspace_id == EVENT_WORKSPACE_NONE) {
        mirabel_slogf(LOGS_WARN, "server lobby-manager: missing workspace for event type %u %s", e->base.type, event_type_str(e->base.type));
        return;
    }
    switch (e->base.type) {
        //TODO use proper lobby error event displayed in gui, instead of logs
        case EVENT_TYPE_LOBBY_CREATE: {
            event_any re;
            if (e->lobby_create.lobby_name == NULL || e->lobby_create.password == NULL) {
                event_create_lobby_cdjl_err(&re, "lobby name or password can not be empty");
                server_workspace_handle_network_send(appi.aserver, e->base.workspace_id, &re);
                break;
            }
            if (strlen(e->lobby_create.lobby_name) < 3) {
                event_create_lobby_cdjl_err(&re, "lobby name < 3 characters");
                server_workspace_handle_network_send(appi.aserver, e->base.workspace_id, &re);
                break;
            }
            char new_lobby_name[SERVER_LOBBY_LOBBYNAME_SIZE];
            if (strlen(e->lobby_create.lobby_name) == 0) {
                //TODO auto generate using username if no name given
                static uint32_t seed = 123;
                fast_prng rng;
                fprng_srand(&rng, seed++);
                const int assigned_length = 5;
                char* str_p = new_lobby_name;
                str_p += sprintf(str_p, "auto");
                for (int i = 0; i < assigned_length; i++) {
                    str_p += sprintf(str_p, "%u", fprng_rand(&rng) % 10);
                }
            } else {
                strncpy(new_lobby_name, e->lobby_create.lobby_name, SERVER_LOBBY_LOBBYNAME_SIZE);
            }
            if (server_lobby_manager_lobby_get_by_name(self, new_lobby_name) != NULL) {
                event_create_lobby_cdjl_err(&re, "lobbyname already exists");
                server_workspace_handle_network_send(appi.aserver, e->base.workspace_id, &re);
                break;
            }
            uint32_t new_lobby_id = server_lobby_manager_lobby_add(self, new_lobby_name, e->lobby_create.password, timestamp_get_ns64()); //TODO make sure that if password is NULL others can freely join by simply leaving the password box as it is
            //TODO actually add the user+workspace to the lobby! +on server add this info into the workspace handler
            event_create_lobby_join(&re, new_lobby_name, e->lobby_create.password, new_lobby_id);
            server_workspace_handle_network_send(appi.aserver, e->base.workspace_id, &re);

        } break;
        case EVENT_TYPE_LOBBY_DESTROY: {
            event_any re;
            server_lobby* del_lobby = server_lobby_manager_lobby_get_by_id(self, e->lobby_base.lobby_id);
            if (del_lobby == NULL) {
                event_create_lobby_cdjl_err(&re, "lobby does not exist");
                server_workspace_handle_network_send(appi.aserver, e->base.workspace_id, &re);
                break;
            }
            event_create_lobby_base(&re, EVENT_TYPE_LOBBY_LEAVE, del_lobby->id);
            //TODO does the remove call destroy on the lobby and actually remove all users?
            server_lobby_manager_lobby_remove(self, del_lobby->id);
            server_workspace_handle_network_send(appi.aserver, e->base.workspace_id, &re);
        } break;
        case EVENT_TYPE_LOBBY_JOIN: {
            event_any re;
            if (e->lobby_join.lobby_name == NULL || e->lobby_join.password == NULL) {
                event_create_lobby_cdjl_err(&re, "lobby name or password can not be empty");
                server_workspace_handle_network_send(appi.aserver, e->base.workspace_id, &re);
                break;
            }
            server_lobby* join_lobby = server_lobby_manager_lobby_get_by_name(self, e->lobby_join.lobby_name);
            if (join_lobby == NULL) {
                event_create_lobby_cdjl_err(&re, "lobby does not exist");
                server_workspace_handle_network_send(appi.aserver, e->base.workspace_id, &re);
                break;
            }
            //TODO error if password incorrect
            //TODO add user+workspace to lobby +on server add this info into the workspace handler
            event_create_lobby_join(&re, join_lobby->lobbyname, e->lobby_join.password, join_lobby->id);
            server_workspace_handle_network_send(appi.aserver, e->base.workspace_id, &re);
        } break;
        case EVENT_TYPE_LOBBY_LEAVE: {
            event_any re;
            if (e->lobby_base.lobby_id == LOBBY_ID_NONE) {
                event_create_lobby_cdjl_err(&re, "can not leave lobby, not part of any lobby");
                server_workspace_handle_network_send(appi.aserver, e->base.workspace_id, &re);
                break;
            }
            server_lobby* leave_lobby = server_lobby_manager_lobby_get_by_id(self, e->lobby_base.lobby_id);
            //TODO remove user from lobby
            event_create_lobby_base(&re, EVENT_TYPE_LOBBY_LEAVE, leave_lobby->id);
            server_workspace_handle_network_send(appi.aserver, e->base.workspace_id, &re);
        } break;
        default: {
            mirabel_slogf(LOGS_WARN, "server lobby-manager: received unexpected event, type: %u %s", e->base.type, event_type_str(e->base.type));
        } break;
    }
}

uint32_t server_lobby_manager_lobby_add(server_lobby_manager* self, const char* lobbyname, const char* password, uint64_t password_salt)
{
    server_lobby new_lobby = (server_lobby){
        .dirty = false,
        .id = next_lobby_id++,
    };
    strncpy(new_lobby.lobbyname, lobbyname, SERVER_LOBBY_LOBBYNAME_SIZE);
    server_lobby_manager_password_hash(new_lobby.password_hash, password, password_salt);
    //TODO server_lobby_add which params do we want in the create function?
    VEC_PUSH(&self->loaded_slots, new_lobby);
    return new_lobby.id;
}

void server_lobby_manager_lobby_remove(server_lobby_manager* self, uint32_t id)
{
    if (server_lobby_manager_lobby_get_by_id(self, id) == NULL) {
        mirabel_slogf(LOGS_WARN, "server lobby-manager: attempting to remove non-existant lobby");
        return;
    }
    //TODO server_lobby_destroy
    size_t remove_slot = server_lobby_manager_lobby_get_by_id(self, id) - self->loaded_slots;
    VEC_REMOVE_SWAP(&self->loaded_slots, remove_slot);
}

void server_lobby_manager_password_hash(uint8_t password_hash[SERVER_LOBBY_PASSWORD_HASH_SIZE], const char* password, uint64_t password_salt)
{
    //TODO //HACK use some crypto hash for password hashing, unfortunately openssl is probably not sensible to ship in the web version?
    for (size_t i = 0; i < SERVER_LOBBY_PASSWORD_HASH_SIZE; i++) {
        password_hash[i] = password_salt >> (8 * (i % sizeof(uint64_t)));
    }
    if (password != NULL) {
        uint32_t* acc = (uint32_t*)password_hash;
        size_t acc_idx = 0;
        const size_t max_acc_idx = SERVER_LOBBY_PASSWORD_HASH_SIZE / sizeof(uint32_t);
        const char* wstr_end = password + strlen(password);
        for (size_t i = 0; i < 64; i++) {
            const char* wstr = password;
            while (wstr < wstr_end) {
                acc[acc_idx % max_acc_idx] *= squirrelnoise5(acc[(acc_idx + 1) % max_acc_idx], *wstr);
                acc_idx += 1;
                acc[acc_idx % max_acc_idx] ^= squirrelnoise5(*wstr, acc[(acc_idx + 1) % max_acc_idx]);
                wstr++;
            }
        }
    }
}
