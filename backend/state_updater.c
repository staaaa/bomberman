#include "state_updater.h"
#include <stdio.h>

void update_gamestate(PlayerMove *move, GameState *gs) {
    for (int i = 0; i < gs->num_players; i++) {
        if (gs->players[i].id == move->player_id) {
            if (move->move_type == MOVED) {
                gs->players[i].pos_x = move->pos_x;
                gs->players[i].pos_y = move->pos_y;
            }
        }
    }
}