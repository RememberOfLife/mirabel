#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "mirabel/engine.h"
#include "mirabel/game.h"

#include "mirabel/engine_wrap.h"
#include "mirabel/frontend.h"
#include "mirabel/game_wrap.h"

namespace Control {

    //TODO block unloading of plugins while any of the provided methods are in use
    // e.g. mark plugin as in use? or just the methods in the catalogue?

    class BaseGameVariantImpl {

      public:

        mutable uint32_t map_idx; //TODO maybe put this in the constructor instead of a mutable member

        bool wrapped;

        union {
            const game_methods* methods;
            const game_wrap* wrap;
        } u;

        BaseGameVariantImpl(const game_methods* methods);
        BaseGameVariantImpl(const game_wrap* wrap);
        ~BaseGameVariantImpl();

        game* new_game(game_init init_info) const;

        const game_methods* get_methods() const;
        const char* get_name() const;

        // wrap the wrapper opts or a general purpose string api for methods
        void create_opts(void** opts) const;
        void display_opts(void* opts) const;
        void destroy_opts(void* opts) const;

        // wrap runtime create/display/destroy
        void create_runtime(game* rgame, void** opts) const;
        void display_runtime(game* rgame, void* opts) const;
        void destroy_runtime(void* opts) const;

        friend bool operator<(const BaseGameVariantImpl& lhs, const BaseGameVariantImpl& rhs);
    };

    class BaseGameVariant {

      public:

        mutable uint32_t map_idx; //TODO maybe put this in the constructor instead of a mutable member

        std::string name;
        mutable std::set<BaseGameVariantImpl> impls;

        BaseGameVariant(const char* name);
        ~BaseGameVariant();

        friend bool operator<(const BaseGameVariant& lhs, const BaseGameVariant& rhs);
    };

    class BaseGame {

      public:

        mutable uint32_t map_idx; //TODO maybe put this in the constructor instead of a mutable member

        std::string name;
        mutable std::set<BaseGameVariant> variants;

        BaseGame(const char* name);
        ~BaseGame();

        friend bool operator<(const BaseGame& lhs, const BaseGame& rhs);
    };

    class FrontendImpl {

      public:

        mutable uint32_t map_idx; //TODO maybe put this in the constructor instead of a mutable member

        const frontend_methods* methods;

        FrontendImpl(const frontend_methods* methods);
        ~FrontendImpl();

        frontend* new_frontend(frontend_display_data* dd, void* load_opts) const;

        const char* get_name() const;

        void create_opts(void** opts) const;
        void display_opts(void* opts) const;
        void destroy_opts(void* opts) const;

        friend bool operator<(const FrontendImpl& lhs, const FrontendImpl& rhs);
    };

    class EngineImpl {

      public:

        mutable uint32_t map_idx; //TODO maybe put this in the constructor instead of a mutable member

        bool wrapped;

        union {
            const engine_methods* methods;
            const engine_wrap* wrap;
        } u;

        EngineImpl(const engine_methods* methods);
        EngineImpl(const engine_wrap* wrap);
        ~EngineImpl();

        const engine_methods* get_methods() const;
        const char* get_name() const;

        // wrap the wrapper opts or a general purpose string api for methods
        void create_opts(void** opts) const;
        void display_opts(void* opts) const;
        void destroy_opts(void* opts) const;

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

        // metagui display comboboxes use this for random access to the set objects
        uint32_t lookup_idx = 1; // start at 1, 0 is invalid/none
        //TODO why need iterators here?
        std::unordered_map<uint32_t, std::set<BaseGame>::iterator> game_lookup;
        std::unordered_map<uint32_t, std::set<BaseGameVariant>::iterator> variant_lookup;
        std::unordered_map<uint32_t, std::set<BaseGameVariantImpl>::iterator> impl_lookup;
        std::unordered_map<uint32_t, std::set<FrontendImpl>::iterator> frontend_lookup;
        std::unordered_map<uint32_t, std::set<EngineImpl>::iterator> engine_lookup;

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

        bool get_game_impl_idx(const char* base_name, const char* variant_name, const char* impl_name, uint32_t* base_idx, uint32_t* variant_idx, uint32_t* impl_idx);
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

} // namespace Control
