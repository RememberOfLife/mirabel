#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "surena/game.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct move_history_s move_history;
struct move_history_s {
    // the move how we got here, the root node of a move history tree is always PLAYER_NONE and MOVE_NONE
    player_id player;
    move_code move;
    char* move_str;

    // binary tree to represent that a node can have many children
    //TODO this can not serve transposition merging into the same subtree (in that case, reachable sub graph) again, but its currently not required anyway
    move_history* parent;
    move_history* left_child;
    move_history* right_sibling;
    uint32_t idx_in_parent;

    uint32_t selected_child; // idx, ~0 is none
    bool is_split;

    // readonly, these are updated by the manipulation functions
    uint32_t split_height; // childless node has height 0; does not consider the left most child (main line), because IT will be padded out by this height
    uint32_t width; // SUM of widths of children, terminal node has width 1; equal to number of leaf nodes from here
};

void move_history_create(move_history* h);

// inserts the player and move combination as a child for the given history, returns a pointer to the newly created move node
// if the combination already exists, it simply returns the pointer to the already existing history
// in all cases the created/existing node is also selected for h
move_history* move_history_insert(move_history* h, player_id player, move_code move);

// recursively sets all its parents selected_child indices to the path pointing to this history, also unsets the selected_child idx for itself
void move_history_select(move_history* h);

void move_history_promote(move_history* h, bool to_main);

void move_history_demote(move_history* h);

// size_t move_history_measure(move_history* h);
// void move_history_serialize(move_history* h, char* buf); //TODO into human readable?

// move_history* move_history_deserialize(const char* buf, size_t len); //TODO

// recursively free this and all its child histories
// if a parent exists, this will remove the destroyed history sub tree from the parents child list
void move_history_destroy(move_history* h);

#ifdef __cplusplus
}
#endif
