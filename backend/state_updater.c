#include "state_updater.h"
#include <stdio.h>
#include <stdlib.h>

#define BOMB_FUSE_TIME 60           // Time before explosion in game ticks
#define EXPLOSION_DURATION 30       // Time explosion stays visible

void assign_player_spawn(GameState* gs, int player_id) {
    int map_width = gs->map.width;
    int map_height = gs->map.height;

    switch (player_id) {
        case 0:
            gs->players[player_id].pos_x = 0;
            gs->players[player_id].pos_y = 0;
            break;
        case 1:
            gs->players[player_id].pos_x = map_width * 40 - 40;
            gs->players[player_id].pos_y = 0;
            break;
        case 2:
            gs->players[player_id].pos_x = 0;
            gs->players[player_id].pos_y = map_height * 40 - 40;
            break;
        case 3:
            gs->players[player_id].pos_x = map_width * 40 - 40;
            gs->players[player_id].pos_y = map_height * 40 - 40;
            break;
        default:
            gs->players[player_id].pos_x = 0;
            gs->players[player_id].pos_y = 0;
            break;
    }
}

void place_bomb(GameState *gs, int player_id, int x, int y) {
    for (int i = 0; i < gs->num_players; i++) {
        if (gs->players[i].id == player_id && gs->players[i].alive) {
            // Check if player can place more bombs
            if (gs->players[i].curr_bomb_num < gs->players[i].max_bomb_num) {
                // Find an inactive bomb slot
                for (int j = 0; j < gs->players[i].max_bomb_num; j++) {
                    if (!gs->players[i].bombs[j].active) {
                        // Place bomb at player's grid position (not at x,y which are raw coords)
                        int grid_x = gs->players[i].pos_x / 40;  // 40 is GRID_SIZE
                        int grid_y = gs->players[i].pos_y / 40;
                        
                        // Check if tile is empty
                        if (gs->map.tile_arr[grid_y][grid_x].type == EMPTY) {
                            printf("Player %d placed bomb at grid %d,%d\n", player_id, grid_x, grid_y);
                            gs->players[i].bombs[j].pos_x = grid_x;
                            gs->players[i].bombs[j].pos_y = grid_y;
                            gs->players[i].bombs[j].range = 1;
                            gs->players[i].bombs[j].fuse_time = BOMB_FUSE_TIME;
                            gs->players[i].bombs[j].active = 1;
                            gs->players[i].bombs[j].owner_id = player_id;
                            gs->players[i].bombs[j].explosion_duration = 0;
                            gs->players[i].curr_bomb_num++;
                            
                            // Mark tile as bomb
                            gs->map.tile_arr[grid_y][grid_x].type = BOMB;
                            break;
                        }
                    }
                }
            }
            break;
        }
    }
}

void explode_bomb(GameState *gs, Bomb *bomb) {
    int x = bomb->pos_x;
    int y = bomb->pos_y;
    int range = bomb->range;
    
    // Mark original bomb position as explosion
    gs->map.tile_arr[y][x].type = EXPLOSION;
    
    // Check all 8 directions (including diagonals)
    for (int dy = -range; dy <= range; dy++) {
        for (int dx = -range; dx <= range; dx++) {
            int new_x = x + dx;
            int new_y = y + dy;
            
            // Skip if out of bounds
            if (new_x < 0 || new_x >= gs->map.width || new_y < 0 || new_y >= gs->map.height) {
                continue;
            }
            
            // Skip if it's an unbreakable wall
            if (gs->map.tile_arr[new_y][new_x].type == UNBREAKABLE_WALL) {
                continue;
            }
            
            // Destroy breakable walls or update to explosion
            if (gs->map.tile_arr[new_y][new_x].type == BREAKABLE_WALL) {
                gs->map.tile_arr[new_y][new_x].type = EMPTY;

                // Award 1 point to the bomb owner
                for (int p = 0; p < gs->num_players; p++) {
                    if (gs->players[p].id == bomb->owner_id) {
                        gs->players[p].points++;
                        break;
                    }
                }
            } else {
                gs->map.tile_arr[new_y][new_x].type = EXPLOSION;
            }


            
            // Check if any player is in explosion range
            for (int i = 0; i < gs->num_players; i++) {
                int player_grid_x = gs->players[i].pos_x / 40;
                int player_grid_y = gs->players[i].pos_y / 40;
                
                if (player_grid_x == new_x && player_grid_y == new_y && gs->players[i].alive) {
                    gs->players[i].alive = 0;  // Kill player
                    // Award 3 points to the bomb owner
                    for (int p = 0; p < gs->num_players; p++) {
                        if (gs->players[p].id == bomb->owner_id && p != i) {
                            gs->players[p].points += 3;
                            break;
                        }
                    }
                }

            }
        }
    }
    
    // Set bomb to explosion state
    bomb->explosion_duration = EXPLOSION_DURATION;
    bomb->fuse_time = 0;
}

void update_bombs(GameState *gs) {
    // Called every game tick to update all bombs states
    for (int i = 0; i < gs->num_players; i++) {
        for (int j = 0; j < gs->players[i].max_bomb_num; j++) {
            Bomb *bomb = &gs->players[i].bombs[j];
            
            if (bomb->active) {
                if (bomb->fuse_time > 0) {
                    // Countdown fuse
                    bomb->fuse_time--;
                    
                    // Explode when timer reaches 0
                    if (bomb->fuse_time == 0) {
                        printf("Bomb exploded at %d,%d\n", bomb->pos_x, bomb->pos_y);
                        explode_bomb(gs, bomb);
                    }
                } else if (bomb->explosion_duration > 0) {
                    // Countdown explosion duration
                    bomb->explosion_duration--;
                    
                    // Clear explosion when timer reaches 0
                    if (bomb->explosion_duration == 0) {
                        int x = bomb->pos_x;
                        int y = bomb->pos_y;
                        int range = bomb->range;
                        
                        // Reset explosion tiles to empty
                        for (int dy = -range; dy <= range; dy++) {
                            for (int dx = -range; dx <= range; dx++) {
                                int new_x = x + dx;
                                int new_y = y + dy;
                                
                                if (new_x >= 0 && new_x < gs->map.width && new_y >= 0 && new_y < gs->map.height) {
                                    if (gs->map.tile_arr[new_y][new_x].type == EXPLOSION) {
                                        gs->map.tile_arr[new_y][new_x].type = EMPTY;
                                    }
                                }
                            }
                        }
                        
                        printf("Explosion cleared at %d,%d\n", bomb->pos_x, bomb->pos_y);
                        // Reset bomb state
                        bomb->active = 0;
                        gs->players[i].curr_bomb_num--;
                    }
                }
            }
        }
    }
}


void update_gamestate(PlayerMove *move, GameState *gs) {
    if (move->move_type == RESTART) {
        for (int i = 0; i < gs->num_players; i++) {
            if (gs->players[i].id == move->player_id) {
                // Reset player state
                gs->players[i].alive = 1;
                assign_player_spawn(gs, i);
                gs->players[i].points = 0;
                gs->players[i].curr_bomb_num = 0;
                
                // Deactivate all bombs and clear map tiles
                for (int j = 0; j < gs->players[i].max_bomb_num; j++) {
                    Bomb *bomb = &gs->players[i].bombs[j];
                    if (bomb->active) {
                        if (bomb->fuse_time > 0) { // Bomb is in fuse phase
                            // Clear the bomb tile
                            if (bomb->pos_x >= 0 && bomb->pos_x < gs->map.width &&
                                bomb->pos_y >= 0 && bomb->pos_y < gs->map.height) {
                                gs->map.tile_arr[bomb->pos_y][bomb->pos_x].type = EMPTY;
                            }
                        }
                        bomb->active = 0;
                    }
                }
                break;
            }
        }
    } else {
        // Existing code for MOVED and PLACED_BOMB
        for (int i = 0; i < gs->num_players; i++) {
            if (gs->players[i].id == move->player_id && gs->players[i].alive) {
                if (move->move_type == MOVED) {
                    gs->players[i].pos_x = move->pos_x;
                    gs->players[i].pos_y = move->pos_y;
                } else if (move->move_type == PLACED_BOMB) {
                    place_bomb(gs, move->player_id, move->pos_x, move->pos_y);
                }
            }
        }
    }
    
    // Update bombs every tick
    update_bombs(gs);
}