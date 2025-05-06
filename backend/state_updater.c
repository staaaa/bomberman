#include "state_updater.h"
#include <stdio.h>
#include <stdlib.h>

#define BOMB_FUSE_TIME 60           // Time before explosion in game ticks
#define EXPLOSION_DURATION 30       // Time explosion stays visible

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
            } else {
                gs->map.tile_arr[new_y][new_x].type = EXPLOSION;
            }
            
            // Check if any player is in explosion range
            for (int i = 0; i < gs->num_players; i++) {
                int player_grid_x = gs->players[i].pos_x / 40;
                int player_grid_y = gs->players[i].pos_y / 40;
                
                if (player_grid_x == new_x && player_grid_y == new_y && gs->players[i].alive) {
                    gs->players[i].alive = 0;  // Kill player
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
    
    // Update bombs every tick
    update_bombs(gs);
}