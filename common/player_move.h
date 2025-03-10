#ifndef PLAYER_MOVE_H
#define PLAYER_MOVE_H

enum MoveType{
  IDLE,
  PLACED_BOMB,
  MOVED
};

typedef struct PlayerMove{
  enum MoveType move_type;
  int pos_x;
  int pox_y;
}PlayerMove;

#endif
