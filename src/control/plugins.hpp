#pragma once

#include <cstdint>

namespace Control {

    class PluginManager {

        //TODO history manager should only set the guithread state, client will still use newest one for networking, except if user distributed to network?
        // on which state should the engine search, main line or selected state
        // in general what is the network interaction for the history manager

        //TODO this will store all loaded and considered plugins and offer functionality to load/unload plugins, adding/removing them to/from their respective catalogues
        // this has to stay gui agnostic so the server can also just use with scan and load all

        //TODO func check in folder and add to list of viable plugins
        //TODO func load/unload

        //TODO ability to save and open state and history

    };

}
