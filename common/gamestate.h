#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "bomb.h"
#include "map.h"

#define MAX_PLAYERS 10

typedef struct Player{
    int id;
    int max_bomb_num;
    int curr_bomb_num;
    int pos_x;
    int pos_y;
    int alive;
    int points;
    Bomb bombs[3]; // Assuming max 3 bombs per player
} Player;

typedef struct GameState {
    int num_players;
    Player players[MAX_PLAYERS];
    Map map;
} GameState;

#endif