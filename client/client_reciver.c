#include "../common/gamestate.h"
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void *receive_data(void *arg)
{
  int client_socket = *(int *)arg;
  GameState buffer;
  int bytes_recived;
  while (1)
  {
    bytes_recived = recv(client_socket, &buffer, sizeof(buffer), 0);
    if (bytes_recived <= 0)
    {
      printf("Wystąpił błąd odbioru danych. \n");
      break;
    }
    else
    {
      system("clear");
      printf("Otrzymano od serwera: %d \n", buffer.dummy);
    }
  }
  close(client_socket);
  return 0;
}
