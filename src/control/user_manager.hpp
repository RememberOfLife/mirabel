#pragma once

#include <cstddef>
#include <cstdbool>
#include <cstdint>

namespace Control {

    static const uint64_t USER_ID_NONE = 0;
    static const uint64_t USER_ID_MASK_GUEST = (uint64_t)1 << 63;
    static const uint64_t USER_ID_MASK_ID = ~USER_ID_MASK_GUEST;

    bool is_guest(uint64_t user_id);

    class UserManager {
      private:

        uint64_t next_guest_id; // if is 0 then disallow guests

      public:

        UserManager();
        ~UserManager();
        //TODO func for auth isnt here, but use a separate auth manager which handles all auth events, and add a new event to signal the network manager that a certain user now possesses a certain user id, ??still need some way to get user id from client, and vice versa??
        //TODO ^^^ make sure users do not sign in twice at the same time!
        //TODO how to retrieve certain info about clients? cache somehow, or COPY everytime?!
        //TODO create new user/guest / retrieve info
        //TODO manip user info?

        //TODO need diff sig for guests alltogether?
        //TODO what really is the use of passworded guests?
        // returns USER_ID_NONE if the user already exists
        uint64_t create_user(const char* username, const char* password, bool guest);

        uint64_t get_user_by_name(const char* username);

        // returns USER_ID_NONE if invalid
        uint64_t authenticate_user(const char* username, const char* password, bool guest);
    };

} // namespace Control
