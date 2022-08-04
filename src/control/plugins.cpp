#include <algorithm>
#include <cstdint>
#include <dirent.h>
#include <dlfcn.h>
#include <unordered_set>
#include <vector>

#include "imgui.h"
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

    static const size_t OPTS_WRAP_STR_SIZE = 512;

    BaseGameVariantImpl::BaseGameVariantImpl(const game_methods* methods):
        wrapped(false)
    {
        u.methods = methods;
    }

    BaseGameVariantImpl::BaseGameVariantImpl(const game_wrap* wrap):
        wrapped(true)
    {
        u.wrap = wrap;
    }

    BaseGameVariantImpl::~BaseGameVariantImpl()
    {}

    const char* BaseGameVariantImpl::get_name() const
    {
        return (wrapped ? u.wrap->backend->impl_name : u.methods->impl_name);
    }

    void BaseGameVariantImpl::create_opts(void** opts)
    {
        if (wrapped) {
            u.wrap->opts_create(opts);
        } else {
            *opts = malloc(OPTS_WRAP_STR_SIZE);
        }
    }

    void BaseGameVariantImpl::display_opts(void* opts)
    {
        if (wrapped) {
            u.wrap->opts_display(opts);
        } else {
            ImGui::InputText("opts", (char*)opts, OPTS_WRAP_STR_SIZE);
        }
    }

    void BaseGameVariantImpl::destroy_opts(void* opts)
    {
        if (wrapped) {
            u.wrap->opts_destroy(opts);
        } else {
            free(opts);
        }
    }

    bool operator<(const BaseGameVariantImpl& lhs, const BaseGameVariantImpl& rhs)
    {
        return strcmp(lhs.get_name(), rhs.get_name()) < 0;
    }

    BaseGameVariant::BaseGameVariant(const char* name):
        name(name)
    {}

    BaseGameVariant::~BaseGameVariant()
    {}

    bool operator<(const BaseGameVariant& lhs, const BaseGameVariant& rhs)
    {
        return strcmp(lhs.name.c_str(), rhs.name.c_str()) < 0;
    }

    BaseGame::BaseGame(const char* name):
        name(name)
    {}

    BaseGame::~BaseGame()
    {}

    bool operator<(const BaseGame& lhs, const BaseGame& rhs)
    {
        return strcmp(lhs.name.c_str(), rhs.name.c_str()) < 0;
    }

    FrontendImpl::FrontendImpl(const frontend_methods* methods):
        methods(methods)
    {}

    FrontendImpl::~FrontendImpl()
    {}

    const char* FrontendImpl::get_name() const
    {
        return methods->frontend_name;
    }

    void FrontendImpl::create_opts(void** opts)
    {
        if (methods->features.options) {
            methods->opts_create(opts);
        }
    }

    void FrontendImpl::display_opts(void* opts)
    {
        if (methods->features.options) {
            methods->opts_display(opts);
        } else {
            ImGui::TextDisabled("<no options>");
        }
    }

    void FrontendImpl::destroy_opts(void* opts)
    {
        if (methods->features.options) {
            methods->opts_destroy(opts);
        }
    }

    bool operator<(const FrontendImpl& lhs, const FrontendImpl& rhs)
    {
        return strcmp(lhs.methods->frontend_name, rhs.methods->frontend_name) < 0;
    }

    EngineImpl::EngineImpl(const engine_methods* methods):
        wrapped(false)
    {
        u.methods = methods;
    }

    EngineImpl::EngineImpl(const engine_wrap* wrap):
        wrapped(true)
    {
        u.wrap = wrap;
    }

    EngineImpl::~EngineImpl()
    {}

    const char* EngineImpl::get_name() const
    {
        return (wrapped ? u.wrap->backend.engine_name : u.methods->engine_name);
    }

    void EngineImpl::create_opts(void** opts)
    {
        if (wrapped) {
            u.wrap->opts_create(opts);
        } else {
            *opts = malloc(OPTS_WRAP_STR_SIZE);
        }
    }

    void EngineImpl::display_opts(void* opts)
    {
        if (wrapped) {
            u.wrap->opts_display(opts);
        } else {
            ImGui::InputText("opts", (char*)opts, OPTS_WRAP_STR_SIZE);
        }
    }
    
    void EngineImpl::destroy_opts(void* opts)
    {
        if (wrapped) {
            u.wrap->opts_destroy(opts);
        } else {
            free(opts);
        }
    }

    bool operator<(const EngineImpl& lhs, const EngineImpl& rhs)
    {
        return strcmp(lhs.get_name(), rhs.get_name()) < 0;
    }

    PluginManager::PluginManager(bool defaults, bool persist_plugins):
        persist_plugins(persist_plugins)
    {
        if (defaults) {
            //TODO load inbuilt games, frontends, engines
        }
        if (persist_plugins) {
            //TODO load plugins persistently, i.e. load all that were loaded on last quit, if they still exist
        }
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
                        if (strcmp(plugins[i].filename.c_str(), dp->d_name) == 0) {
                            skip = true;
                        }
                    }
                    if (!skip) {
                        plugins.push_back(plugin_file{
                            .filename = dp->d_name,
                            .loaded = false,
                        });
                    }
                }
            } else if (dp->d_type == DT_DIR) {
                //TODO search the folder for the relevant main plugin file and load it with the name of the folder
                // search locations are: base of the folder, build dir, bin dir
                //TODO possibly use some plugin description file for base info from the plugins root directory?
                //-> i.e. a file containing name, version, and paths to the different plugin files it may provide
            }
            dp = readdir(dir);
        }
        closedir(dir);
    }

    void PluginManager::load_plugin(int idx)
    {
        if (idx < 0 || idx > plugins.size()) {
            return;
        }
        plugin_file& the_plugin = plugins[idx];

        char pluginpath[512]; //TODO will overflow
        sprintf(pluginpath, "../plugins/%s", the_plugin.filename.c_str());

        void* dll_handle = dlopen(pluginpath, RTLD_LAZY);
        if (dll_handle == NULL) {
            return;
        }

        //TODO handle partial abort cases where some provided methods might already be filled, for now just clear any remaining provided methods
        the_plugin.provided_game_methods.clear();
        the_plugin.provided_game_wraps.clear();
        the_plugin.provided_frontends.clear();
        the_plugin.provided_engine_methods.clear();
        the_plugin.provided_engine_wraps.clear();

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
            the_plugin.provided_game_methods.resize(method_cnt, NULL);
            get_methods(&method_cnt, &the_plugin.provided_game_methods.front());
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
            the_plugin.provided_game_wraps.resize(method_cnt, NULL);
            get_methods(&method_cnt, &the_plugin.provided_game_wraps.front());
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
            the_plugin.provided_frontends.resize(method_cnt, NULL);
            get_methods(&method_cnt, &the_plugin.provided_frontends.front());
            if (method_cnt == 0) {
                return;
            }
        }

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
            the_plugin.provided_engine_methods.resize(method_cnt, NULL);
            get_methods(&method_cnt, &the_plugin.provided_engine_methods.front());
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
            the_plugin.provided_engine_wraps.resize(method_cnt, NULL);
            get_methods(&method_cnt, &the_plugin.provided_engine_wraps.front());
            if (method_cnt == 0) {
                return;
            }
        }

        the_plugin.dll_handle = dll_handle;

        // insert provided methods in to the catalogues
        // if the name exists already then skip it and remove it from the provided list so it doesnt get removed on unloading later on
        //TODO remove reuse
        for (int i = the_plugin.provided_game_methods.size() - 1; i >= 0; i--) {
            if (add_game_methods(the_plugin.provided_game_methods[i]) == false) {
                the_plugin.provided_game_methods.erase(the_plugin.provided_game_methods.begin()+i);
            }
        }
        for (int i = the_plugin.provided_game_wraps.size() - 1; i >= 0; i--) {
            if (add_game_wrap(the_plugin.provided_game_wraps[i]) == false) {
                the_plugin.provided_game_wraps.erase(the_plugin.provided_game_wraps.begin()+i);
            }
        }
        for (int i = the_plugin.provided_frontends.size() - 1; i >= 0; i--) {
            if (add_frontend(the_plugin.provided_frontends[i]) == false) {
                the_plugin.provided_frontends.erase(the_plugin.provided_frontends.begin()+i);
            }
        }
        for (int i = the_plugin.provided_engine_methods.size() - 1; i >= 0; i--) {
            if (add_engine_methods(the_plugin.provided_engine_methods[i]) == false) {
                the_plugin.provided_engine_methods.erase(the_plugin.provided_engine_methods.begin()+i);
            }
        }
        for (int i = the_plugin.provided_engine_wraps.size() - 1; i >= 0; i--) {
            if (add_engine_wrap(the_plugin.provided_engine_wraps[i]) == false) {
                the_plugin.provided_engine_wraps.erase(the_plugin.provided_engine_wraps.begin()+i);
            }
        }

        the_plugin.loaded = true;
    }

    void PluginManager::unload_plugin(int idx)
    {
        if (idx < 0 || idx > plugins.size()) {
            return;
        }
        plugin_file& the_plugin = plugins[idx];

        // again, much reuse

        for (const game_methods* el : the_plugin.provided_game_methods) {
            remove_game_methods(el);
        }
        for (const game_wrap* el : the_plugin.provided_game_wraps) {
            remove_game_wrap(el);
        }
        for (const frontend_methods* el : the_plugin.provided_frontends) {
            remove_frontend(el);
        }
        for (const engine_methods* el : the_plugin.provided_engine_methods) {
            remove_engine_methods(el);
        }
        for (const engine_wrap* el : the_plugin.provided_engine_wraps) {
            remove_engine_wrap(el);
        }
        the_plugin.provided_game_methods.clear();
        the_plugin.provided_game_wraps.clear();
        the_plugin.provided_frontends.clear();
        the_plugin.provided_engine_methods.clear();
        the_plugin.provided_engine_wraps.clear();

        void* dll_handle = the_plugin.dll_handle;

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

        the_plugin.loaded = false;
    }

    const game_methods* PluginManager::get_game_methods(const char* base_name, const char* variant_name, const char* impl_name)
    {
        std::set<BaseGame>::iterator itrG = game_catalogue.find(base_name);
        if (itrG == game_catalogue.end()) {
            return NULL;
        }
        std::set<BaseGameVariant>::iterator itrV = itrG->variants.find(variant_name);
        if (itrV == itrG->variants.end()) {
            return NULL;
        }
        for (BaseGameVariantImpl itrI : itrV->impls) {
            if (strcmp(itrI.get_name(), impl_name) == 0) {
                if (itrI.wrapped) {
                    return itrI.u.wrap->backend;
                } else {
                    return itrI.u.methods;
                }
            }
        }
        return NULL;
    }

    bool PluginManager::add_game_impl(const char* game_name, const char* variant_name, BaseGameVariantImpl impl)
    {
        std::pair<std::set<BaseGame>::iterator, bool> itrG = game_catalogue.emplace(game_name);
        std::pair<std::set<BaseGameVariant>::iterator, bool> itrV = itrG.first->variants.emplace(variant_name);
        return itrV.first->impls.insert(impl).second;
    }

    bool PluginManager::add_game_methods(const game_methods* methods)
    {
        return add_game_impl(methods->game_name, methods->variant_name, BaseGameVariantImpl(methods));
    }

    bool PluginManager::add_game_wrap(const game_wrap* wrap)
    {
        return add_game_impl(wrap->backend->game_name, wrap->backend->variant_name, BaseGameVariantImpl(wrap));
    }

    bool PluginManager::add_frontend(const frontend_methods* methods)
    {
        return frontend_catalogue.emplace(methods).second;
    }

    bool PluginManager::add_engine_methods(const engine_methods* methods)
    {
        return engine_catalogue.emplace(methods).second;
    }

    bool PluginManager::add_engine_wrap(const engine_wrap* wrap)
    {
        return engine_catalogue.emplace(wrap).second;
    }

    void PluginManager::remove_game_impl(const char* game_name, const char* variant_name, BaseGameVariantImpl impl)
    {
        std::set<BaseGame>::iterator itrG = game_catalogue.find(game_name);
        if (itrG == game_catalogue.end()) {
            return;
        }
        std::set<BaseGameVariant>::iterator itrV = itrG->variants.find(variant_name);
        if (itrV == itrG->variants.end()) {
            return;
        }
        if (itrV->impls.erase(impl) == 0) {
            return;
        }
        if (itrV->impls.size() == 0) {
            itrG->variants.erase(itrV);
        }
        if (itrG->variants.size() == 0) {
            game_catalogue.erase(itrG);
        }
    }

    void PluginManager::remove_game_methods(const game_methods* methods)
    {
        remove_game_impl(methods->game_name, methods->variant_name, BaseGameVariantImpl(methods));
    }

    void PluginManager::remove_game_wrap(const game_wrap* wrap)
    {
        remove_game_impl(wrap->backend->game_name, wrap->backend->variant_name, BaseGameVariantImpl(wrap));
    }

    void PluginManager::remove_frontend(const frontend_methods* methods)
    {
        frontend_catalogue.erase(methods);
    }

    void PluginManager::remove_engine_methods(const engine_methods* methods)
    {
        engine_catalogue.erase(methods);
    }

    void PluginManager::remove_engine_wrap(const engine_wrap* wrap)
    {
        engine_catalogue.erase(wrap);
    }

}
