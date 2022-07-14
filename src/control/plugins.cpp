#include <cstdint>
#include <dirent.h>
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
#include "meta_gui/meta_gui.hpp"

#include "control/plugins.hpp"

namespace Control {

    PluginManager::PluginManager()
    {
        log = MetaGui::log_register("plugins");
        //TODO load inbuilt games, frontends, engines
        //TODO load plugins persistently, i.e. load all that were loaded on last quit, if they still exist
    }

    PluginManager::~PluginManager()
    {
        //TODO unload all plugins
        for (int i = 0; i < plugins.size(); i++) {
            unload_plugin(i);
        }
        MetaGui::log_unregister(log);
    }

    void PluginManager::detect_plugins()
    {
        for (int i = plugins.size() - 1; i >= 0; i-- ){
            if (plugins[i].loaded == false) {
                plugins.erase(plugins.begin() + i);
            }
        }
        DIR* dir = opendir("../plugins/"); //TODO just like the res paths this should be relative to the location of the binary, or set via some config to another path
        struct dirent* dp = readdir(dir);
        while (dp)
        {
            if (dp->d_type == DT_REG) { //TODO want to allow symlinks?
                if (strstr(dp->d_name, ".so") != NULL || strstr(dp->d_name, ".dll") != NULL) {
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

    void PluginManager::load_plugin(int idx)
    {
        MetaGui::logf("#I loading plugin: %s", plugins[idx].filename);
        //TODO try to load all types of plugins from the file, make sure to insert them into the catalogues in alphabetical order
        plugins[idx].loaded = true;
    }

    void PluginManager::unload_plugin(int idx)
    {
        MetaGui::logf("#I unloading plugin: %s", plugins[idx].filename);
        //TODO unload all types of plugins where at least one object was provided
        plugins[idx].loaded = false;
    }

}
