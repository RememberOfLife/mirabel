#pragma once

#include <cstdint>
#include <set>
#include <unordered_set>
#include <vector>

#include "surena/engine.h"
#include "surena/game.h"

#include "mirabel/engine_wrap.h"
#include "mirabel/frontend.h"
#include "mirabel/game_wrap.h"

namespace Control {

    //TODO implement comparison operators for all of these?

    //TODO block unloading of plugins while any of the provided methods are in use
    // e.g. mark plugin as in use? or just the methods in the catalogue?

    class BaseGameVariantImpl {

        public:

            bool wrapped;
            union {
                const game_methods* methods;
                const game_wrap* wrap;
            } u;

            BaseGameVariantImpl(const game_methods* methods);
            BaseGameVariantImpl(const game_wrap* wrap);
            ~BaseGameVariantImpl();

            // wrap the wrapper opts or a general purpose string api for methods
            void create_opts(void** opts);
            void display_opts(void* opts);
            void destroy_opts(void* opts);

            //TODO runtime display

    };

    class BaseGameVariant {

        public:

            const char* name;
            std::set<BaseGameVariantImpl> impls;

            BaseGameVariant(const char* name);
            ~BaseGameVariant();

    };

    class BaseGame {

        public:

            const char* name;
            std::set<BaseGameVariant> variants;

            BaseGame(const char* name);
            ~BaseGame();

    };

    class EngineImpl {

        public:
        
            bool wrapped;
            union {
                const engine_methods* methods;
                const engine_wrap* wrap;
            } u;

            EngineImpl(const engine_methods* methods);
            EngineImpl(const engine_wrap* wrap);
            ~EngineImpl();

            // wrap the wrapper opts or a general purpose string api for methods
            void create_opts(void** opts);
            void display_opts(void* opts);
            void destroy_opts(void* opts);

    };

    //TODO the plugin manager now also functions as the catalogue manager, but there is no real use in separating them right now
    class PluginManager {

        //TODO logging for the plugin manager, but it also needs to work on the server, requires logging solution

        public:

            // the server plugins dont contain gui code they just load engine_methods and game_methods, keep both lists and offer two search methods
            std::set<BaseGame> game_catalogue;
            std::set<const frontend_methods*> frontend_catalogue; //TODO want extra wrapper like game and engine?
            std::set<EngineImpl> engine_catalogue;

            struct plugin_file {
                const char* filename;
                bool loaded;
                void* dll_handle;

                // need to store these here to easily remove them from the catalogues
                std::unordered_set<const engine_methods*> provided_engine_methods;
                std::unordered_set<const game_methods*> provided_game_methods;
                std::unordered_set<const engine_wrap*> provided_engine_wraps;
                std::unordered_set<const frontend_methods*> provided_frontends;
                std::unordered_set<const game_wrap*> provided_game_wraps;
            };
            std::vector<plugin_file> plugins;

            PluginManager();
            ~PluginManager();

            void detect_plugins();
            void load_plugin(int idx);
            void unload_plugin(int idx);

            const game_methods* get_game_methods(const char* base_name, const char* variant_name);
            //TODO get compatible for frontends and engines?

            // return true if the impl was added, false if dupe
            bool add_engine_methods(const engine_methods* methods);
            bool add_game_methods(const game_methods* methods);
            bool add_engine_wrap(const engine_wrap* wrap);
            bool add_frontend(const frontend_methods* methods);
            bool add_game_wrap(const game_wrap* wrap);

            // does nothing if nonexistant
            void remove_engine_methods(const engine_methods* methods);
            void remove_game_methods(const game_methods* methods);
            void remove_engine_wrap(const engine_wrap* wrap);
            void remove_frontend(const frontend_methods* methods);
            void remove_game_wrap(const game_wrap* wrap);

            // return false if they need be swapped, true if already in correct order (assuming a list going from left to right)
            static bool compare_engine_methods(const engine_methods* left, const engine_methods* right);
            static bool compare_game_methods(const game_methods* left, const game_methods* right);
            static bool compare_engine_wrap(const engine_wrap* left, const engine_wrap* right);
            static bool compare_frontend(const frontend_methods* left, const frontend_methods* right);
            static bool compare_game_wrap(const game_wrap* left, const game_wrap* right);

    };

}
