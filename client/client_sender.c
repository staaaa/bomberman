#include "../common/player_move.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <ncurses.h>

void *send_data(void *arg)
{
    int client_socket = *(int *)arg;
    PlayerMove move = {IDLE, 0, 0}; // temp
    enum MoveType last_move_type = IDLE;
    initscr();
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    while (1)
    {
        int ch = getch();
        if (ch == ERR)
        {
            usleep(5000);
            continue;
        }
        if (ch == KEY_UP)
        {
            move.move_type = PLACED_BOMB;
        }
        else if (ch == KEY_DOWN)
        {
            move.move_type = MOVED;
        }
        else
        {
            move.move_type = IDLE;
        }

        if (move.move_type != last_move_type)
        {
            if (send(client_socket, &move, sizeof(move), 0) <= 0)
            {
                printw("Błąd podczas wysyłania danych. \n");
                break;
            }
            last_move_type = move.move_type;
        }
    }
    close(client_socket);
    return NULL;
}
