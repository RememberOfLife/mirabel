#include <algorithm>
#include <cstdint>
#include <dirent.h>
#include <dlfcn.h>
#include <unordered_set>
#include <vector>

#include "surena/engine_plugin.h"
#include "surena/engine.h"
#include "surena/game_plugin.h"
#include "surena/game.h"

#include "mirabel/engine_wrap_plugin.h"
#include "mirabel/engine_wrap.h"
#include "mirabel/frontend_plugin.h"
#include "mirabel/frontend.h"
#include "mirabel/game_wrap_plugin.h"
#include "mirabel/game_wrap.h"

#include "control/plugins.hpp"

namespace Control {


    BaseGameVariantImpl::BaseGameVariantImpl(const game_methods* methods)
    {
        //TODO
    }

    BaseGameVariantImpl::BaseGameVariantImpl(const game_wrap* wrap)
    {
        //TODO
    }

    BaseGameVariantImpl::~BaseGameVariantImpl()
    {
        //TODO
    }

    void BaseGameVariantImpl::create_opts(void** opts)
    {
        //TODO
    }

    void BaseGameVariantImpl::display_opts(void* opts)
    {
        //TODO
    }

    void BaseGameVariantImpl::destroy_opts(void* opts)
    {
        //TODO
    }

    BaseGameVariant::BaseGameVariant(const char* name)
    {
        //TODO
    }

    BaseGameVariant::~BaseGameVariant()
    {
        //TODO
    }

    BaseGame::BaseGame(const char* name)
    {
        //TODO
    }

    BaseGame::~BaseGame()
    {
        //TODO
    }

    EngineImpl::EngineImpl(const engine_methods* methods)
    {
        //TODO
    }

    EngineImpl::EngineImpl(const engine_wrap* wrap)
    {
        //TODO
    }

    EngineImpl::~EngineImpl()
    {
        //TODO
    }

    void EngineImpl::create_opts(void** opts)
    {
        //TODO
    }

    void EngineImpl::display_opts(void* opts)
    {
        //TODO
    }
    
    void EngineImpl::destroy_opts(void* opts)
    {
        //TODO
    }

    PluginManager::PluginManager()
    {
        //TODO load inbuilt games, frontends, engines
        //TODO load plugins persistently, i.e. load all that were loaded on last quit, if they still exist
    }

    PluginManager::~PluginManager()
    {
        //TODO unload all plugins
        for (int i = 0; i < plugins.size(); i++) {
            unload_plugin(i);
        }
    }

    void PluginManager::detect_plugins()
    {
        for (int i = plugins.size() - 1; i >= 0; i-- ){
            if (plugins[i].loaded == false) {
                plugins.erase(plugins.begin() + i);
            }
        }
        DIR* dir = opendir("../plugins/"); //TODO just like the res paths this should be relative to the location of the binary, or set via some config to another path
        if (dir == NULL) {
            return;
        }
        struct dirent* dp = readdir(dir);
        while (dp)
        {
            if (dp->d_type == DT_REG) { //TODO want to allow symlinks?
                if (strstr(dp->d_name, ".so") != NULL || strstr(dp->d_name, ".dll") != NULL) { //TODO make sure that these occur as the very last thing in the filename
                    bool skip = false;
                    for (int i = 0; i < plugins.size(); i++) {
                        // check that this file isnt already loaded
                        if (strcmp(plugins[i].filename, dp->d_name) == 0) {
                            skip = true;
                        }
                    }
                    if (!skip) {
                        plugins.push_back(plugin_file{
                            .filename = strdup(dp->d_name),
                            .loaded = false,
                        });
                    }
                }
            } else if (dp->d_type == DT_DIR) {
                //TODO search the folder for the relevant main plugin file and load it with the name of the folder
                // search locations are: base of the folder, build dir, bin dir
            }
            dp = readdir(dir);
        }
        closedir(dir);
    }

    //TODO use within game classes
    // returns true if element already existed, in that case element is also not inserted
    // https://stackoverflow.com/a/25524075
    template< typename T, typename PRED >
    bool insert_sorted(std::vector<T>& vec, T const& item, PRED pred)
    {
        auto lb = std::lower_bound(vec.begin(), vec.end(), item, pred);
        if (*lb == item) {
            return true;
        }
        vec.insert(lb, item);
        return false;
    }

    void PluginManager::load_plugin(int idx)
    {
        //TODO clear all provided methods

        char pluginpath[512];
        sprintf(pluginpath, "../plugins/%s", plugins[idx].filename);

        void* dll_handle = dlopen(pluginpath, RTLD_LAZY);
        if (dll_handle == NULL) {
            return;
        }

        /*TODO adapt for impls

        // load engine_methods
        plugin_get_engine_capi_version_t em_version = (plugin_get_engine_capi_version_t)dlsym(dll_handle, "plugin_get_engine_capi_version");
        if (em_version != NULL && em_version() == SURENA_ENGINE_API_VERSION) {
            void (*init)() = (void (*)())dlsym(dll_handle, "plugin_init_engine");
            if (init == NULL) {
                return;
            }
            init();
            plugin_get_engine_methods_t get_methods = (plugin_get_engine_methods_t)dlsym(dll_handle, "plugin_get_engine_methods");
            if (get_methods == NULL) {
                return;
            }
            uint32_t method_cnt;
            get_methods(&method_cnt, NULL);
            if (method_cnt == 0) {
                return;
            }
            plugins[idx].provided_engine_methods.resize(method_cnt, NULL);
            get_methods(&method_cnt, &plugins[idx].provided_engine_methods.front());
            if (method_cnt == 0) {
                return;
            }
        }

        // load game_methods
        plugin_get_game_capi_version_t gm_version = (plugin_get_game_capi_version_t)dlsym(dll_handle, "plugin_get_game_capi_version");
        if (gm_version != NULL && gm_version() == SURENA_GAME_API_VERSION) {
            void (*init)() = (void (*)())dlsym(dll_handle, "plugin_init_game");
            if (init == NULL) {
                return;
            }
            init();
            plugin_get_game_methods_t get_methods = (plugin_get_game_methods_t)dlsym(dll_handle, "plugin_get_game_methods");
            if (get_methods == NULL) {
                return;
            }
            uint32_t method_cnt;
            get_methods(&method_cnt, NULL);
            if (method_cnt == 0) {
                return;
            }
            plugins[idx].provided_game_methods.resize(method_cnt, NULL);
            get_methods(&method_cnt, &plugins[idx].provided_game_methods.front());
            if (method_cnt == 0) {
                return;
            }
        }

        // load engine_wraps
        plugin_get_engine_wrap_capi_version_t ew_version = (plugin_get_engine_wrap_capi_version_t)dlsym(dll_handle, "plugin_get_engine_wrap_capi_version");
        if (ew_version != NULL && ew_version() == MIRABEL_ENGINE_WRAP_API_VERSION) {
            void (*init)() = (void (*)())dlsym(dll_handle, "plugin_init_engine_wrap");
            if (init == NULL) {
                return;
            }
            init();
            plugin_get_engine_wrap_methods_t get_methods = (plugin_get_engine_wrap_methods_t)dlsym(dll_handle, "plugin_get_engine_wrap_methods");
            if (get_methods == NULL) {
                return;
            }
            uint32_t method_cnt;
            get_methods(&method_cnt, NULL);
            if (method_cnt == 0) {
                return;
            }
            plugins[idx].provided_engine_wraps.resize(method_cnt, NULL);
            get_methods(&method_cnt, &plugins[idx].provided_engine_wraps.front());
            if (method_cnt == 0) {
                return;
            }
        }

        // load frontend_methods
        plugin_get_frontend_capi_version_t fe_version = (plugin_get_frontend_capi_version_t)dlsym(dll_handle, "plugin_get_frontend_capi_version");
        if (fe_version != NULL && fe_version() == MIRABEL_FRONTEND_API_VERSION) {
            void (*init)() = (void (*)())dlsym(dll_handle, "plugin_init_frontend");
            if (init == NULL) {
                return;
            }
            init();
            plugin_get_frontend_methods_t get_methods = (plugin_get_frontend_methods_t)dlsym(dll_handle, "plugin_get_frontend_methods");
            if (get_methods == NULL) {
                return;
            }
            uint32_t method_cnt;
            get_methods(&method_cnt, NULL);
            if (method_cnt == 0) {
                return;
            }
            plugins[idx].provided_frontends.resize(method_cnt, NULL);
            get_methods(&method_cnt, &plugins[idx].provided_frontends.front());
            if (method_cnt == 0) {
                return;
            }
        }

        // load game_wraps
        plugin_get_frontend_capi_version_t gw_version = (plugin_get_frontend_capi_version_t)dlsym(dll_handle, "plugin_get_game_wrap_capi_version");
        if (gw_version != NULL && gw_version() == MIRABEL_GAME_WRAP_API_VERSION) {
            void (*init)() = (void (*)())dlsym(dll_handle, "plugin_init_game_wrap");
            if (init == NULL) {
                return;
            }
            init();
            plugin_get_game_wrap_methods_t get_methods = (plugin_get_game_wrap_methods_t)dlsym(dll_handle, "plugin_get_game_wrap_methods");
            if (get_methods == NULL) {
                return;
            }
            uint32_t method_cnt;
            get_methods(&method_cnt, NULL);
            if (method_cnt == 0) {
                return;
            }
            plugins[idx].provided_game_wraps.resize(method_cnt, NULL);
            get_methods(&method_cnt, &plugins[idx].provided_game_wraps.front());
            if (method_cnt == 0) {
                return;
            }
        }

        plugins[idx].dll_handle = dll_handle;

        */

        // insert provided methods in to the catalogues in alphabetical order
        // if the name exists already then skip it and remove it from the provided list so it doesnt get removed on unloading later on

        //TODO

        plugins[idx].loaded = true;
    }

    // https://stackoverflow.com/a/39379632
    template<typename T>
    void remove_intersection(std::vector<T>& a, std::vector<T>& b)
    {
        std::unordered_multiset<T> st;
        st.insert(a.begin(), a.end());
        st.insert(b.begin(), b.end());
        auto predicate = [&st](const T& k){ return st.count(k) > 1; };
        a.erase(std::remove_if(a.begin(), a.end(), predicate), a.end());
    }

    void PluginManager::unload_plugin(int idx)
    {
        // again, much reuse

        /*TODO
        remove_intersection(engine_methods_catalogue, plugins[idx].provided_engine_methods);
        remove_intersection(game_methods_catalogue, plugins[idx].provided_game_methods);
        remove_intersection(engine_wrap_catalogue, plugins[idx].provided_engine_wraps);
        remove_intersection(frontend_catalogue, plugins[idx].provided_frontends);
        remove_intersection(game_wrap_catalogue, plugins[idx].provided_game_wraps);
        */

        void* dll_handle = plugins[idx].dll_handle;

        void (*em_cleanup)() = (void (*)())dlsym(dll_handle, "plugin_cleanup_engine");
        if (em_cleanup == NULL) {
            return;
        }
        em_cleanup();

        void (*gm_cleanup)() = (void (*)())dlsym(dll_handle, "plugin_cleanup_game");
        if (gm_cleanup == NULL) {
            return;
        }
        gm_cleanup();

        void (*ew_cleanup)() = (void (*)())dlsym(dll_handle, "plugin_cleanup_engine_wrap");
        if (ew_cleanup == NULL) {
            return;
        }
        ew_cleanup();

        void (*fe_cleanup)() = (void (*)())dlsym(dll_handle, "plugin_cleanup_frontend");
        if (fe_cleanup == NULL) {
            return;
        }
        fe_cleanup();

        void (*gw_cleanup)() = (void (*)())dlsym(dll_handle, "plugin_cleanup_game_wrap");
        if (gw_cleanup == NULL) {
            return;
        }
        gw_cleanup();

        dlclose(dll_handle);

        plugins[idx].loaded = false;
    }


    const game_methods* PluginManager::get_game_methods(const char* base_name, const char* variant_name)
    {
        //TODO
        return NULL;
    }

    bool PluginManager::add_engine_methods(const engine_methods* methods)
    {
        //TODO
        return false;
    }

    bool PluginManager::add_game_methods(const game_methods* methods)
    {
        //TODO
        return false;
    }

    bool PluginManager::add_engine_wrap(const engine_wrap* wrap)
    {
        //TODO
        return false;
    }

    bool PluginManager::add_frontend(const frontend_methods* methods)
    {
        //TODO
        return false;
    }

    bool PluginManager::add_game_wrap(const game_wrap* wrap)
    {
        //TODO
        return false;
    }

    void PluginManager::remove_engine_methods(const engine_methods* methods)
    {
        //TODO
    }

    void PluginManager::remove_game_methods(const game_methods* methods)
    {
        //TODO
    }

    void PluginManager::remove_engine_wrap(const engine_wrap* wrap)
    {
        //TODO
    }

    void PluginManager::remove_frontend(const frontend_methods* methods)
    {
        //TODO
    }

    void PluginManager::remove_game_wrap(const game_wrap* wrap)
    {
        //TODO
    }

    bool PluginManager::compare_engine_methods(const engine_methods* left, const engine_methods* right)
    {
        //TODO
        return false;
    }

    bool PluginManager::compare_game_methods(const game_methods* left, const game_methods* right)
    {
        //TODO
        return false;
    }

    bool PluginManager::compare_engine_wrap(const engine_wrap* left, const engine_wrap* right)
    {
        //TODO
        return false;
    }

    bool PluginManager::compare_frontend(const frontend_methods* left, const frontend_methods* right)
    {
        //TODO
        return false;
    }

    bool PluginManager::compare_game_wrap(const game_wrap* left, const game_wrap* right)
    {
        //TODO
        return false;
    }

}
