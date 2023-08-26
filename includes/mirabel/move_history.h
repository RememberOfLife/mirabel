#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mirabel/game.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint64_t SURENA_MOVE_HISTORY_API_VERSION = 4;

typedef struct move_history_s move_history;

struct move_history_s {
    blob* sync_data; // rosa_vec<blob> the sync data imported before the move is applied
    // the move how we got here, the root node of a move history tree is always PLAYER_NONE and MOVE_NONE
    player_id player;
    move_data_sync move;
    char* move_str;

    //TODO cache game serialization every few nodes

    // binary tree to represent that a node can have many children
    //TODO this can not serve transposition merging into the same subtree (in that case, reachable sub graph) again, but its currently not required anyway
    move_history* parent;
    move_history* left_child;
    move_history* right_sibling;
    uint32_t idx_in_parent;

    uint32_t selected_child; // idx, ~0 is none
    bool is_split;

    // these are updated by the manipulation functions
    uint32_t height; // childless node has height 0; height is max of child heights
    uint32_t split_height; // effective height of this node, for use when parent wants to split and pad out main line by this; max of children but does not consider the left most child (main line)
    uint32_t width; // SUM of widths of children, terminal node has width 1; equal to number of leaf nodes from here
};

move_history* move_history_create();

// inserts the (sync_data, player, move) combination as a child for the given history, returns a pointer to the newly created move node
// if the combination already exists, it simply returns the pointer to the already existing history (this only compares the move, not the sync data!)
// in all cases the created/existing node is also selected for h
// sync_data is a rosa_vec<blob>
move_history* move_history_insert(move_history* h, blob* sync_data, player_id player, move_data_sync move, const char* move_str);

// recursively sets all its parents selected_child indices to the path pointing to this history, also unsets the selected_child idx for itself
void move_history_select(move_history* h);

void move_history_promote(move_history* h, bool to_main);

void move_history_demote(move_history* h);

void move_history_split(move_history* h, bool split);

// size_t move_history_measure(move_history* h);
// void move_history_serialize(move_history* h, char* buf); //TODO into human readable?

// move_history* move_history_deserialize(const char* buf, size_t len); //TODO

// recursively free this and all its child histories
// if a parent exists, this will remove the destroyed history sub tree from the parents child list
void move_history_destroy(move_history* h);

#ifdef __cplusplus
}
#endif
