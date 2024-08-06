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

#include "mirabel/server/user_manager.h"

//TODO using appi.aserver is ugly here..

uint64_t next_user_id = 1;

bool server_user_manager_create(server_user_manager* self)
{
    VEC_CREATE(&self->loaded_slots, 8);
}

void server_user_manager_destroy(server_user_manager* self)
{
    VEC_DESTROY(&self->loaded_slots);
}

server_user* server_user_manager_user_get_by_id(server_user_manager* self, uint64_t id)
{
    //TODO use map to make this faster
    for (size_t search_idx = 0; search_idx < VEC_LEN(&self->loaded_slots); search_idx++) {
        if (self->loaded_slots[search_idx].id == id) {
            return &self->loaded_slots[search_idx];
        }
    }
    return NULL;
}

server_user* server_user_manager_user_get_by_name(server_user_manager* self, const char* username)
{
    //TODO use map to make this faster
    for (size_t search_idx = 0; search_idx < VEC_LEN(&self->loaded_slots); search_idx++) {
        if (strcmp(self->loaded_slots[search_idx].username, username) == 0) {
            return &self->loaded_slots[search_idx];
        }
    }
    return NULL;
}

void server_user_manager_handle_event(server_user_manager* self, event_any* e)
{
    switch (e->base.type) {
        case EVENT_TYPE_NETWORK_CONNECTION_OPEN: {
            // client wants to have the authinfo, serve it
            event_any re;
            event_create_user_auth_info(&re, true, NULL, NULL);
            server_connection_network_send(appi.aserver, e->base.connection_id, &re);
        } break;
        case EVENT_TYPE_USER_AUTH_INFO: {
            // client wants to auth with given credentials, send back authn or authfail
            event_any re;
            //TODO save guest names and check for dupes
            if (!e->user_auth_info.is_guest) {
                event_create_user_auth_reject(&re, "user logins not accepted");
                server_connection_network_send(appi.aserver, e->base.connection_id, &re);
                break;
            }
            if (e->user_auth_info.username == NULL) {
                event_create_user_auth_reject(&re, "name NULL");
                server_connection_network_send(appi.aserver, e->base.connection_id, &re);
                break;
            }
            // validate that username uses only allowed characters
            for (size_t i = 0; i < strlen(e->user_auth_info.username); i++) {
                if (!strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-", e->user_auth_info.username[i])) {
                    event_create_user_auth_reject(&re, "name contains illegal characters");
                    server_connection_network_send(appi.aserver, e->base.connection_id, &re);
                    break;
                }
            }
            if (strlen(e->user_auth_info.username) > 0 && strlen(e->user_auth_info.username) < 3) {
                event_create_user_auth_reject(&re, "name < 3 characters");
                server_connection_network_send(appi.aserver, e->base.connection_id, &re);
                break;
            }
            if (strlen(e->user_auth_info.username) == 0) {
                free(e->user_auth_info.username);
                static uint32_t seed = 123;
                fast_prng rng;
                fprng_srand(&rng, seed++);
                const int assigned_length = 5;
                const int guestname_length = 6 + assigned_length;
                e->user_auth_info.username = (char*)malloc(guestname_length);
                char* str_p = e->user_auth_info.username;
                str_p += sprintf(str_p, "Guest");
                for (int i = 0; i < assigned_length; i++) {
                    str_p += sprintf(str_p, "%u", fprng_rand(&rng) % 10);
                }
            }
            if (server_user_manager_user_get_by_name(self, e->user_auth_info.username) != NULL) {
                event_create_user_auth_reject(&re, "username already exists");
                server_connection_network_send(appi.aserver, e->base.connection_id, &re);
                break;
            }
            appi.aserver->connections[e->base.connection_id].authn_user_id = server_user_manager_user_add(self, true, e->user_auth_info.username, "xxxxxxxx", timestamp_get_ns64());
            event_create_user_auth_info(&re, true, e->user_auth_info.username, NULL);
            server_connection_network_send(appi.aserver, e->base.connection_id, &re);
        } break;
        case EVENT_TYPE_USER_AUTH_REJECT: {
            // client wants to logout but keep the connection, we tell them we logged them out
            server_user_manager_user_remove(self, appi.aserver->connections[e->base.connection_id].authn_user_id);
            appi.aserver->connections[e->base.connection_id].authn_user_id = USER_ID_NONE;
            event_any re;
            event_create_user_auth_reject(&re, NULL);
            server_connection_network_send(appi.aserver, e->base.connection_id, &re);
            event_create_user_auth_info(&re, true, NULL, NULL);
            server_connection_network_send(appi.aserver, e->base.connection_id, &re);
        } break;
        default: {
            mirabel_slogf(LOGS_WARN, "server user-manager: received unexpected event, type: %u %s", e->base.type, event_type_str(e->base.type));
        } break;
    }
}

uint64_t server_user_manager_user_add(server_user_manager* self, bool is_guest, const char* username, const char* password, uint64_t password_salt)
{
    server_user new_user = (server_user){
        .dirty = false,
        .id = next_user_id++,
        .is_guest = is_guest,
    };
    strncpy(new_user.username, username, SERVER_USER_USERNAME_SIZE);
    server_user_manager_password_hash(new_user.password_hash, password, password_salt);
    VEC_PUSH(&self->loaded_slots, new_user);
    return new_user.id;
}

void server_user_manager_user_remove(server_user_manager* self, uint64_t id)
{
    if (server_user_manager_user_get_by_id(self, id) == NULL) {
        mirabel_slogf(LOGS_WARN, "server user-manager: attempting to remove non-existant user");
        return;
    }
    size_t remove_slot = server_user_manager_user_get_by_id(self, id) - self->loaded_slots;
    VEC_REMOVE_SWAP(&self->loaded_slots, remove_slot);
}

void server_user_manager_password_hash(uint8_t password_hash[SERVER_USER_PASSWORD_HASH_SIZE], const char* password, uint64_t password_salt)
{
    //TODO //HACK use some crypto hash for password hashing, unfortunately openssl is probably not sensible to ship in the web version?
    for (size_t i = 0; i < SERVER_USER_PASSWORD_HASH_SIZE; i++) {
        password_hash[i] = password_salt >> (8 * (i % sizeof(uint64_t)));
    }
    uint32_t* acc = (uint32_t*)password_hash;
    size_t acc_idx = 0;
    const size_t max_acc_idx = SERVER_USER_PASSWORD_HASH_SIZE / sizeof(uint32_t);
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
