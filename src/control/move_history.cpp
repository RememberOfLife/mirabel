#include <cstdbool>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

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
        .width = 1,
    };
}

move_history* move_history_insert(move_history* h, player_id player, move_code move)
{
    move_history** ip = &h->left_child;
    uint32_t idx_in_parent = 0;
    move_history* lp = h->left_child;
    while (lp) {
        if (lp->player == player && lp->move == move) {
            h->selected_child = lp->idx_in_parent;
            lp->selected_child = UINT32_MAX;
            return lp;
        }
        if (lp->right_sibling == NULL) {
            ip = &lp->right_sibling;
            idx_in_parent = lp->idx_in_parent + 1;
        }
        lp = lp->right_sibling;
    }
    // move node was not found, create and insert at ip, reuse lp as new pointer
    lp = (move_history*)malloc(sizeof(move_history));
    move_history_create(lp);
    lp->player = player;
    lp->move = move;
    lp->parent = h;
    lp->idx_in_parent = idx_in_parent;
    *ip = lp;
    h->selected_child = idx_in_parent;
    // propagate height and width increase up through the tree
    bool prop_width = (idx_in_parent > 0);
    move_history* bp = lp;
    if (prop_width) {
        lp->width--;
    }
    while (bp) {
        if (prop_width) {
            bp->width++;
        }
        //TODO propagate split height
        // if (bp->parent && bp->idx_in_parent > 0) {
        //     bp->parent->split_height++;
        // }
        bp = bp->parent;
    }
    return lp;
}

void move_history_select(move_history* h)
{
    h->selected_child = UINT32_MAX;
    move_history* lp = h;
    while (lp->parent) {
        if (lp->parent->selected_child == lp->idx_in_parent) {
            break; // early quit if part of the existing path can be reused
        }
        lp->parent->selected_child = lp->idx_in_parent;
        lp = lp->parent;
    }
}

void move_history_promote(move_history* h, bool to_main)
{
    //TODO set height of parent tree correctly
    // promote to main promote along the entire line
    move_history* promo = h;
    while (promo->parent) {

        if (promo->idx_in_parent == 0) {
            continue;
        }
        move_history* double_left_sibling = NULL;
        move_history* left_sibling = promo->parent->left_child;
        if (left_sibling == promo) {
            continue;
        }
        // search for the preceeding node in in the list
        uint32_t idx_skip_len = 0;
        while (left_sibling->right_sibling != promo) {
            double_left_sibling = left_sibling;
            if (to_main) {
                left_sibling->idx_in_parent++;
                idx_skip_len++;
            }
            left_sibling = left_sibling->right_sibling;
        }
        if (to_main || double_left_sibling == NULL) {
            // link as local main
            left_sibling->right_sibling = promo->right_sibling;
            promo->right_sibling = promo->parent->left_child;
            promo->parent->left_child = promo;
        } else {
            // swap links
            left_sibling->right_sibling = promo->right_sibling;
            promo->right_sibling = left_sibling;
            double_left_sibling->right_sibling = promo;
        }
        // adjust idx in parent
        left_sibling->idx_in_parent++;
        promo->idx_in_parent -= 1 + idx_skip_len;
        promo->parent->selected_child = promo->idx_in_parent;

        if (!to_main) {
            return;
        }
        promo = promo->parent;
    }
}

void move_history_demote(move_history* h)
{
    //TODO set height of parent tree correctly
    if (h->parent == NULL || h->right_sibling == NULL) {
        return;
    }
    // find left sibling
    move_history* left_sibling = h->parent->left_child;
    if (left_sibling == h) {
        // we're the left most child, swap immediately
        h->parent->left_child = h->right_sibling;
        h->right_sibling = h->parent->left_child->right_sibling;
        h->parent->left_child->right_sibling = h;
        h->parent->left_child->idx_in_parent--;
        h->idx_in_parent++;
    } else {
        // search for the preceeding node in in the list
        while (left_sibling->right_sibling != h) {
            left_sibling = left_sibling->right_sibling;
        }
        // swap h and its succ
        left_sibling->right_sibling = h->right_sibling;
        h->right_sibling = h->right_sibling->right_sibling;
        left_sibling->right_sibling->right_sibling = h;
        // adjust idx in parent
        left_sibling->right_sibling->idx_in_parent--;
        h->idx_in_parent++;
    }
    h->parent->selected_child = h->idx_in_parent;
}

void move_history_destroy(move_history* h)
{
    if (h->parent->selected_child == h->idx_in_parent) {
        h->parent->selected_child = UINT32_MAX;
    } else if (h->idx_in_parent < h->parent->selected_child) {
        h->parent->selected_child--;
    }
    //TODO set height of parent tree correctly
    if (h->parent) {
        move_history* left_sibling = h->parent->left_child;
        if (left_sibling == h) {
            // we're the left most child, unlink immediately
            h->parent->left_child = h->right_sibling;
        } else {
            // search for the preceeding node in in the list
            while (left_sibling->right_sibling != h) {
                left_sibling = left_sibling->right_sibling;
            }
            // unlink h
            left_sibling->right_sibling = h->right_sibling;
            // decrement idx in parent for all others
            while (left_sibling->right_sibling) {
                left_sibling = left_sibling->right_sibling;
                left_sibling->idx_in_parent--;
            }
        }
        // propagate width decrease up through the tree
        if (h->parent->width > 1) {
            uint32_t remove_width = h->width;
            move_history* bp = h;
            while (bp) {
                bp->width -= remove_width;
                bp = bp->parent;
            }
        }
        h->parent = NULL;
        h->right_sibling = NULL;
    }
    //WARNING if h has no parent, this will also destroy all right siblings of h
    // rotate the tree inplace into a linked list on the parent pointers, to avoid allocations in destroy
    move_history* del_head = h;
    while (true) {
        if (del_head->left_child) {
            del_head->left_child->parent = del_head->parent;
            del_head->parent = del_head->left_child;
            del_head->left_child = NULL;
        } else if (del_head->right_sibling) {
            del_head->right_sibling->parent = del_head->parent;
            del_head->parent = del_head->right_sibling;
            del_head->right_sibling = NULL;
        } else if (del_head->parent) {
            // delete current node
            move_history* del_free = del_head;
            del_head = del_head->parent;
            free(del_free->move_str);
            free(del_free);
        } else {
            break;
        }
    }
    // delete last node
    free(del_head->move_str);
    free(del_head);
}

#ifdef __cplusplus
}
#endif
