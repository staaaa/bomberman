#include "gamestate.h"
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 50000
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
GameState gs = {1};

void *handle_client(void *arg)
{
  int client_socket = *(int *)arg;
  free(arg);
  pthread_mutex_lock(&mutex);
  gs.dummy++;
  pthread_mutex_unlock(&mutex);
  char message[BUFFER_SIZE];
  sprintf(message, "Wartość zmiennej dummy w gamestate wynosi: %d\n", gs.dummy);

  send(client_socket, message, strlen(message), 0);

  close(client_socket);
  return NULL;
}

int main()
{
  socklen_t clilen;
  // we create a socket that works in IPv4 family - AF_INET
  // we also declare that we will be reading information in stream - SOCK_STREAM
  // if we would want to read informations in chunks we would use SOCK_DGRAM
  // third argument is 0 - there we could declare what type of transportation layer (protocol)
  // we want to use for communication
  // we leave it as 0 because kernel as default selects TCP/IP
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0)
  {
    printf("Couldn't create a server socket. Exiting app...");
    exit(1);
  }

  // this is a buffer for reading/writing from/to client
  char buffer[BUFFER_SIZE];

  // struct sockaddr_in is a struct that contains information
  // about ip address, port, family of ip adresses (IPv4/IPv6)
  // we are using AF_INET for IPv4
  struct sockaddr_in server_addr, client_addr;

  // we are seting the sin_zero to all zeros
  // sin_zero is only used as a way of alligning the data - with that the struct
  // sockaddr_in is 16 bytes - SHORT + SHORT + LONG + CHAR[8]
  memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));
  server_addr.sin_family = AF_INET;         // IPv4 family
  server_addr.sin_port = htons(PORT);       // setting port, htons converts short to network byte order (to big endian)
  server_addr.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY sets the IP address to the machine adress

  if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    printf("Couldn't bind the socket to the address");
    exit(1);
  }

  // we are telling the socket to listen, the 5 is a max num of clients in queue
  listen(server_socket, 5);
  printf("Serwer nasłuchuje na porcie %d...\n", PORT);

  int *client_socket;
  while (1)
  {
    clilen = sizeof(client_addr);
    client_socket = malloc(sizeof(int));

    if (!client_socket)
    {
      printf("Wystąpił bład w alokacji deskryptoru socketu klienta");
      continue;
    }

    // accept blocks the while and waits for the client to connect, then create a client socket
    *client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &clilen);
    if (*client_socket < 0)
    {
      printf("Wystąpił błąd w accept");
      continue;
    }
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, handle_client, client_socket) != 0)
    {
      printf("Wystąpił błąd przy tworzeniu wątku...");
      free(client_socket);
      continue;
    }
    printf("Przyjęto klienta.\n");
  }
  free(client_socket);
  close(server_socket);
  return 0;
}
