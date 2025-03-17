#include "../common/gamestate.h"
#include "../common/player_move.h"
#include "state_updater.h"
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 50000
#define MAX_PLAYERS 10
#define BUFFER_SIZE 1024

pthread_mutex_t gs_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cl_mutex = PTHREAD_MUTEX_INITIALIZER;

GameState gs;
int client_list[MAX_PLAYERS];
int num_clients = 0;

void add_client(int client_socket){
  pthread_mutex_lock(&cl_mutex);
  if(num_clients < MAX_PLAYERS){
    client_list[num_clients] = client_socket;
    num_clients++;
  }
  else{
    close(client_socket);
  }
  pthread_mutex_unlock(&cl_mutex);
}

void remove_client(int client_socket){
  pthread_mutex_lock(&cl_mutex);
    for (int i = 0; i < num_clients; i++) {
        if (client_list[i] == client_socket) {
            client_list[i] = client_list[num_clients - 1];
            num_clients--;
            break;
        }
    }
    pthread_mutex_unlock(&cl_mutex);
}

void *game_loop(void *arg) {
    (void)arg; // argument nie jest używany
    
    while (1) {
        GameState snapshot = gs;
        pthread_mutex_lock(&cl_mutex);
        for (int i = 0; i < num_clients; i++) {
            if (send(client_list[i],&snapshot , sizeof(snapshot), 0) < 0) {
                printf("Klient się rozłączył. \n");
                close(client_list[i]);
                remove_client(client_list[i]);
                i--;
            }
        }
        pthread_mutex_unlock(&cl_mutex);
        // Tick co około 16ms (~60 FPS)
        usleep(16000);
    }
    return NULL;
}

void* client_handler(void* arg){
  int client_socket = *(int*) arg;
  free(arg);
  PlayerMove buffer;
  while(1){
    int bytes_read = recv(client_socket, &buffer, sizeof(buffer), 0);
    if(bytes_read <= 0){
      printf("Klient rozłączony. \n");
      close(client_socket);
      remove_client(client_socket);
      break;
    }
    pthread_mutex_lock(&gs_mutex);
    update_gamestate(&buffer, &gs);
    pthread_mutex_unlock(&gs_mutex);
  }
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
    printf("Couldn't create a server socket. Exiting app... \n");
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

  //create main thread of game_loop
  pthread_t game_thread;
  if (pthread_create(&game_thread, NULL, game_loop, NULL) != 0) {
    printf("there was an error creating game loop thread. \n"); 
    exit(EXIT_FAILURE);
  }

  int *client_socket;
  while (1)
  {
    clilen = sizeof(client_addr);
    client_socket = malloc(sizeof(int));

    if (!client_socket)
    {
      printf("Wystąpił bład w alokacji deskryptoru socketu klienta. \n");
      continue;
    }

    // accept blocks the while and waits for the client to connect, then create a client socket
    *client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &clilen);
    if (*client_socket < 0)
    {
      printf("Wystąpił błąd w accept. \n");
      continue;
    }
    add_client(*client_socket);
    pthread_t tid;
    if(pthread_create(&tid, NULL, client_handler, client_socket) != 0)
    {
      printf("There was an error creating client thread. \n");
      free(client_socket);
      continue;
    }
    printf("Nowy klient połączony. \n");
    pthread_detach(tid);
  }
  free(client_socket);
  close(server_socket);
  return 0;
}
