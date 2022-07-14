#pragma once

#include <cstdint>
#include <vector>

#include "surena/engine.h"
#include "surena/game.h"

#include "mirabel/engine_wrap.h"
#include "mirabel/frontend.h"
#include "mirabel/game_wrap.h"

namespace Control {

    class PluginManager {

        private:

            uint32_t log;

        public:

            // the server plugins dont contain gui code they just load engine_methods and game_methods, keep both lists and offer two search methods
            std::vector<engine_methods*> engine_methods_catalogue;
            std::vector<game_methods*> game_methods_catalogue;
            std::vector<engine_wrap*> engine_wrap_catalogue;
            std::vector<frontend*> frontend_catalogue;
            std::vector<game_wrap*> game_wrap_catalogue;

            struct plugin_file {
                const char* filename;
                bool loaded;

                // need to store these here to easily remove them from the catalogues
                std::vector<engine_methods*> provided_engine_methods;
                std::vector<game_methods*> provided_game_methods;
                std::vector<engine_wrap*> provided_engine_wrap;
                std::vector<frontend*> provided_frontend;
                std::vector<game_wrap*> provided_game_wrap;
            };
            std::vector<plugin_file> plugins;

            PluginManager();
            ~PluginManager();

            void detect_plugins();
            void load_plugin(int idx);
            void unload_plugin(int idx);

    };

}
