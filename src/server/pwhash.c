#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "rosalia/noise.h"

#include "mirabel/server/pwhash.h"

void password_hash_create(password_hash* self, const char* password, uint64_t password_salt)
{
    //TODO //HACK use some crypto hash for password hashing, unfortunately openssl is probably not sensible to ship in the web version?
    for (size_t i = 0; i < PASSWORD_HASH_SIZE; i++) {
        self->pw_hash[i] = password_salt >> (8 * (i % sizeof(uint64_t)));
    }
    self->pw_salt = password_salt;
    // if we create without a password, then it just stays the same as the seed, thus only the NULL (or "") password will match
    if (password != NULL) {
        uint32_t* acc = (uint32_t*)self->pw_hash;
        size_t acc_idx = 0;
        const size_t max_acc_idx = PASSWORD_HASH_SIZE / sizeof(uint32_t);
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

bool password_hash_test(password_hash* self, const char* test_password)
{
    password_hash test_hash;
    password_hash_create(&test_hash, test_password, self->pw_salt);
    bool match = true;
    for (size_t ci = 0; ci < PASSWORD_HASH_SIZE; ci++) {
        match &= (self->pw_hash[ci] == test_hash.pw_hash[ci]);
    }
    return match;
}
