#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "bomb.h"
#include "map.h"

typedef struct Player{
  int max_bomb_num;
  int curr_bomb_num;
  int pos_x;
  int pos_y;
  Bomb* bomb_state;
}Player;

typedef struct GameState{
  //at start malloc memory for x players
  //whenever player connects create new player
  int dummy;
}GameState;

#endif
