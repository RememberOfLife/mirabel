#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>

#include "rosalia/vector.h"

#include "surena/game.h"

#include "surena/move_history.h"

#ifdef __cplusplus
extern "C" {
#endif

//TODO make split height work as effective height, i.e. during propagation consider max(height, splitheight)

move_history* move_history_create()
{
    move_history* rp = (move_history*)malloc(sizeof(move_history));
    *rp = (move_history){
        .sync_data = NULL,
        .player = PLAYER_NONE,
        .move = (move_data_sync){
            .md = (move_data){
                .cl.len = 0,
                .data = NULL,
            },
            .sync_ctr = SYNC_CTR_DEFAULT,
        },
        .move_str = NULL,
        .parent = NULL,
        .left_child = NULL,
        .right_sibling = NULL,
        .idx_in_parent = UINT32_MAX,
        .selected_child = UINT32_MAX,
        .is_split = false,
        .height = 0,
        .split_height = 0,
        .width = 1,
    };
    return rp;
}

move_history* move_history_insert(move_history* h, blob* sync_data, player_id player, move_data_sync move, const char* move_str)
{
    move_history** ip = &h->left_child;
    uint32_t idx_in_parent = 0;
    move_history* lp = h->left_child;
    while (lp) {
        if (lp->player == player && game_e_move_sync_compare(lp->move, move)) {
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
    lp = move_history_create();
    if (sync_data != NULL) {
        VEC_CREATE(&lp->sync_data, VEC_LEN(&sync_data));
        VEC_PUSH_N(&lp->sync_data, VEC_LEN(&sync_data));
        for (size_t i = 0; i < VEC_LEN(&sync_data); i++) {
            blob_copy(&lp->sync_data[i], &sync_data[i]);
        }
    }
    lp->player = player;
    game_e_move_sync_copy(&lp->move, &move); //TODO this can fail oom
    lp->move_str = move_str != NULL ? strdup(move_str) : NULL;
    lp->parent = h;
    lp->idx_in_parent = idx_in_parent;
    *ip = lp;
    h->selected_child = idx_in_parent;
    // propagate width increase up through the tree
    bool prop_width = (idx_in_parent > 0);
    move_history* bp = lp;
    if (prop_width) {
        lp->width--;
    }
    while (bp) {
        if (prop_width) {
            bp->width++;
        }
        // propagate height increase along the tree
        if (bp->parent && bp->parent->height <= bp->height) {
            bp->parent->height++;
        }
        // split height propagation
        if (bp->parent && bp->idx_in_parent > 0 && bp->parent->split_height <= bp->height) {
            bp->parent->split_height++;
        }
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
    // promote to main promote along the entire line
    move_history* promo = h;
    while (promo->parent) {

        if (promo->idx_in_parent == 0) {
            if (!to_main) {
                return;
            }
            promo = promo->parent;
            continue;
        }
        move_history* double_left_sibling = NULL;
        move_history* left_sibling = promo->parent->left_child;
        if (left_sibling == promo) {
            if (!to_main) {
                return;
            }
            promo = promo->parent;
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
            // set split height of parent tree correctly, only if main line changed, only if size relevant lines moved at all
            if (promo->height + 1 >= promo->parent->split_height || promo->right_sibling->height + 1 >= promo->parent->split_height) {
                uint32_t sph_max = 0;
                move_history* sph = promo->right_sibling;
                while (sph != NULL) {
                    if (sph->height >= sph_max) {
                        sph_max = sph->height + 1;
                    }
                    sph = sph->right_sibling;
                }
                promo->parent->split_height = sph_max;
            }
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
        // set split height of parent tree correctly, only if main line changed, only if size relevant lines moved at all
        if (h->parent->left_child->height + 1 >= h->parent->split_height || h->height + 1 >= h->parent->split_height) {
            uint32_t sph_max = 0;
            move_history* sph = h->parent->left_child->right_sibling;
            while (sph != NULL) {
                if (sph->height >= sph_max) {
                    sph_max = sph->height + 1;
                }
                sph = sph->right_sibling;
            }
            h->parent->split_height = sph_max;
        }
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

void move_history_split(move_history* h, bool split)
{
    if (h->is_split == split) {
        return;
    }
    h->is_split = split;
    //TODO implement, this also needs to propagate split heigh and effective height because of split
}

//TODO temporary fix for the destroy height propagation, put this into a more efficient loop right in the destroy function
void move_history_recalculate_heights(move_history* h)
{
    move_history* lp = h;
    while (lp != NULL) {
        uint32_t h_max = 0;
        uint32_t sph_max = 0;
        move_history* sph = lp->left_child;
        if (sph != NULL) {
            h_max = sph->height + 1;
            sph = sph->right_sibling;
        }
        while (sph != NULL) {
            if (sph->height >= sph_max) {
                sph_max = sph->height + 1;
            }
            sph = sph->right_sibling;
        }
        lp->height = (h_max > sph_max ? h_max : sph_max);
        lp->split_height = sph_max;
        lp = lp->parent;
    }
}

void move_history_destroy(move_history* h)
{
    //TODO set height and split height of parent tree correctly, height propagates up the tree and can consequently change split height
    if (h->parent != NULL) {
        if (h->parent->selected_child == h->idx_in_parent) {
            h->parent->selected_child = UINT32_MAX;
        } else if (h->idx_in_parent < h->parent->selected_child) {
            h->parent->selected_child--;
        }
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
        move_history_recalculate_heights(h->parent);
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
            for (size_t i = 0; i < VEC_LEN(&del_free->sync_data); i++) {
                blob_destroy(&del_free->sync_data[i]);
            }
            VEC_DESTROY(&del_free->sync_data);
            game_e_move_sync_destroy(del_free->move);
            free(del_free->move_str);
            free(del_free);
        } else {
            break;
        }
    }
    // delete last node
    for (size_t i = 0; i < VEC_LEN(&del_head->sync_data); i++) {
        blob_destroy(&del_head->sync_data[i]);
    }
    VEC_DESTROY(&del_head->sync_data);
    game_e_move_sync_destroy(del_head->move);
    free(del_head->move_str);
    free(del_head);
}

#ifdef __cplusplus
}
#endif
