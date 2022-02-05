#pragma once

namespace Util {

    //TODO it might be possible to replace instanceof checks for downcasting gamewraps, with a run time running id system for every registered component 
    template<typename Base, typename T>
    inline bool instanceof(const T* ptr) {
        return dynamic_cast<const Base*>(ptr) != nullptr;
    }

}
