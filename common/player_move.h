#ifndef PLAYER_MOVE_H
#define PLAYER_MOVE_H

enum MoveType {
    IDLE,
    PLACED_BOMB,
    MOVED,
    RESTART
};

typedef struct PlayerMove {
    int player_id;
    enum MoveType move_type;
    int pos_x;
    int pos_y;
} PlayerMove;

#endif
