#include "state_updater.h"

void update_gamestate(PlayerMove *move, GameState *gs)
{
  if (move->move_type == PLACED_BOMB)
  {
    gs->dummy = 1;
  }
  else if (move->move_type == MOVED)
  {
    gs->dummy = -1;
  }
}
