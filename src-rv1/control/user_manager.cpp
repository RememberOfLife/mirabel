#include <cstddef>
#include <cstdbool>
#include <cstdint>

#include "control/user_manager.hpp"

namespace Control {

    bool is_guest(uint64_t user_id)
    {
        return (user_id & USER_ID_MASK_GUEST);
    }

    UserManager::UserManager()
    {
        //TODO
    }

    UserManager::~UserManager()
    {
        //TODO
    }

} // namespace Control
