#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct password_hash_s {
    uint8_t pw_hash[16];
    uint64_t pw_salt;
} password_hash;

static const size_t PASSWORD_HASH_SIZE = sizeof(((password_hash){}).pw_hash);

void password_hash_create(password_hash* self, const char* password, uint64_t password_salt);

// returns true iff the test_password matches the password that was used to create this hash
//NOTE: for security reasons you should time the entire login handling process and always only return after a constant time; this is to slow down brute force attacks, and not leak information about the comparison through a timing sidechannel
bool password_hash_test(password_hash* self, const char* test_password);

#ifdef __cplusplus
}
#endif
