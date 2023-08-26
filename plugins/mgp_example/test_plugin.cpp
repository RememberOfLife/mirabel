/*
    compile from the project root with:
    g++ -g -fPIC -shared src/test_plugin.cpp src/games/tictactoe.cpp lib/rosalia/src/impl/noise.c lib/rosalia/src/impl/rand.c lib/rosalia/src/impl/semver.c lib/rosalia/src/impl/serialization.c src/game.c -Iincludes -Ilib/rosalia/includes -o build/test_plugin.so
    this provides standard tictactoe as a test for the plugin loading system
    unfortunately it seems that g++ currently doesn't support part of the C features used in serialization.h, so this doesn't actually work
*/

#include "mirabel/games/tictactoe.h"
#include "mirabel/game_plugin.h"
#include "mirabel/game.h"

uint64_t plugin_get_game_capi_version()
{
    return MIRABEL_GAME_API_VERSION;
}

void plugin_init_game()
{}

void plugin_get_game_methods(uint32_t* count, const game_methods** methods)
{
    *count = 1;
    if (methods == NULL) {
        return;
    }
    methods[0] = &tictactoe_standard_gbe;
}

void plugin_cleanup_game()
{}
