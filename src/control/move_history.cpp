#include <cstdbool>
#include <cstddef>
#include <cstdint>

#include "surena/game.h"

#include "mirabel/move_history.h"

#ifdef __cplusplus
extern "C" {
#endif

void move_history_create(move_history* h)
{
    *h = (move_history){
        .player = PLAYER_NONE,
        .move = MOVE_NONE,
        .move_str = NULL,
        .parent = NULL,
        .left_child = NULL,
        .right_sibling = NULL,
        .idx_in_parent = UINT32_MAX,
        .selected_child = UINT32_MAX,
        .is_split = false,
        .height = 0,
        .width = 1,
    };
}

move_history* move_history_insert(move_history* h, player_id player, move_code move)
{
    //TODO
    return NULL;
}

void move_history_select(move_history* h)
{
    //TODO
}

void move_history_promote(move_history* h, bool to_main)
{
    //TODO
}

void move_history_demote(move_history* h)
{
    //TODO
}

void move_history_destroy(move_history* h)
{
    //TODO
}

#ifdef __cplusplus
}
#endif
