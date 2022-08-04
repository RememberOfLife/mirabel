#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "surena/engine.h"
#include "surena/game.h"

#include "mirabel/engine_wrap.h"
#include "mirabel/frontend.h"
#include "mirabel/game_wrap.h"

namespace Control {

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

            const char* get_name() const;

            // wrap the wrapper opts or a general purpose string api for methods
            void create_opts(void** opts);
            void display_opts(void* opts);
            void destroy_opts(void* opts);

            //TODO runtime display

            friend bool operator<(const BaseGameVariantImpl& lhs, const BaseGameVariantImpl& rhs);

    };

    class BaseGameVariant {

        public:

            std::string name;
            mutable std::set<BaseGameVariantImpl> impls;

            BaseGameVariant(const char* name);
            ~BaseGameVariant();

            friend bool operator<(const BaseGameVariant& lhs, const BaseGameVariant& rhs);

    };

    class BaseGame {

        public:

            std::string name;
            mutable std::set<BaseGameVariant> variants;

            BaseGame(const char* name);
            ~BaseGame();

            friend bool operator<(const BaseGame& lhs, const BaseGame& rhs);

    };

    class FrontendImpl {

        public:

            const frontend_methods* methods;

            FrontendImpl(const frontend_methods* methods);
            ~FrontendImpl();

            const char* get_name() const;

            void create_opts(void** opts);
            void display_opts(void* opts);
            void destroy_opts(void* opts);

            friend bool operator<(const FrontendImpl& lhs, const FrontendImpl& rhs);

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

            const char* get_name() const;

            // wrap the wrapper opts or a general purpose string api for methods
            void create_opts(void** opts);
            void display_opts(void* opts);
            void destroy_opts(void* opts);

            friend bool operator<(const EngineImpl& lhs, const EngineImpl& rhs);

    };

    //TODO the plugin manager now also functions as the catalogue manager, but there is no real use in separating them right now
    class PluginManager {

        //TODO logging for the plugin manager, but it also needs to work on the server, requires logging solution

        private:

            bool persist_plugins;

        public:

            // the server plugins dont contain gui code they just load engine_methods and game_methods, keep both lists and offer two search methods
            std::set<BaseGame> game_catalogue;
            std::set<FrontendImpl> frontend_catalogue;
            std::set<EngineImpl> engine_catalogue;

            struct plugin_file {
                std::string filename;
                bool loaded;
                void* dll_handle;

                // need to store these here to easily remove them from the catalogues
                std::vector<const game_methods*> provided_game_methods;
                std::vector<const game_wrap*> provided_game_wraps;
                std::vector<const frontend_methods*> provided_frontends;
                std::vector<const engine_methods*> provided_engine_methods;
                std::vector<const engine_wrap*> provided_engine_wraps;
            };
            std::vector<plugin_file> plugins;

            PluginManager(bool defaults, bool persist_plugins);
            ~PluginManager();

            void detect_plugins();
            void load_plugin(int idx);
            void unload_plugin(int idx);

            const game_methods* get_game_methods(const char* base_name, const char* variant_name, const char* impl_name);
            //TODO get compatible for frontends and engines?

            // return true if the impl was added, false if dupe
            bool add_game_impl(const char* game_name, const char* variant_name, BaseGameVariantImpl impl);
            bool add_game_methods(const game_methods* methods);
            bool add_game_wrap(const game_wrap* wrap);
            bool add_frontend(const frontend_methods* methods);
            bool add_engine_methods(const engine_methods* methods);
            bool add_engine_wrap(const engine_wrap* wrap);

            // does nothing if nonexistant
            void remove_game_impl(const char* game_name, const char* variant_name, BaseGameVariantImpl impl);
            void remove_game_methods(const game_methods* methods);
            void remove_game_wrap(const game_wrap* wrap);
            void remove_frontend(const frontend_methods* methods);
            void remove_engine_methods(const engine_methods* methods);
            void remove_engine_wrap(const engine_wrap* wrap);

    };

}
