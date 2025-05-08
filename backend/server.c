#include "../common/gamestate.h"
#include "../common/player_move.h"
#include "../common/map.h"
#include "state_updater.h"
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "game.pb-c.h" // Wygenerowany plik protobuf-c

#define PORT 50000
#define MAX_PLAYERS 4
#define BUFFER_SIZE 1024

pthread_mutex_t gs_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cl_mutex = PTHREAD_MUTEX_INITIALIZER;

GameState gs;
int client_list[MAX_PLAYERS];
int num_clients = 0;

void cleanup_map(Map *map)
{
  for (int y = 0; y < map->height; y++)
  {
    free(map->tile_arr[y]);
  }
  free(map->tile_arr);
}

// Funkcja do wysyłania danych przez protobuf
int send_protobuf_message(int socket, const uint8_t *buffer, size_t length)
{
  // Wysłanie długości wiadomości (4 bajty)
  uint32_t network_len = htonl((uint32_t)length);
  if (send(socket, &network_len, sizeof(network_len), 0) < 0)
  {
    perror("Error sending message length");
    return -1;
  }

  // Wysłanie właściwej wiadomości
  if (send(socket, buffer, length, 0) < 0)
  {
    perror("Error sending message");
    return -1;
  }

  return 0;
}

// Funkcja do odbierania danych przez protobuf
uint8_t *recv_protobuf_message(int socket, size_t *length)
{
  // Odbieranie długości wiadomości
  uint32_t network_len;
  if (recv(socket, &network_len, sizeof(network_len), 0) != sizeof(network_len))
  {
    perror("Error receiving message length");
    return NULL;
  }

  // Konwersja długości z network byte order
  uint32_t msg_len = ntohl(network_len);

  // Alokacja bufora dla wiadomości
  uint8_t *buffer = malloc(msg_len);
  if (!buffer)
  {
    perror("Failed to allocate memory for message");
    return NULL;
  }

  // Odbieranie właściwej wiadomości
  size_t bytes_received = 0;
  while (bytes_received < msg_len)
  {
    ssize_t curr_recv = recv(socket, buffer + bytes_received, msg_len - bytes_received, 0);
    if (curr_recv <= 0)
    {
      perror("Error receiving message data");
      free(buffer);
      return NULL;
    }
    bytes_received += curr_recv;
  }

  *length = msg_len;
  return buffer;
}

void add_client(int client_socket)
{
  pthread_mutex_lock(&cl_mutex);
  if (num_clients < MAX_PLAYERS)
  {
    client_list[num_clients] = client_socket;

    gs.players[num_clients].id = num_clients;
    assign_player_spawn(&gs, num_clients);
    gs.players[num_clients].max_bomb_num = 3;
    gs.players[num_clients].curr_bomb_num = 0;
    gs.players[num_clients].alive = 1;
    gs.players[num_clients].points = 0;

    // Initialize bombs
    for (int j = 0; j < gs.players[num_clients].max_bomb_num; j++)
    {
      gs.players[num_clients].bombs[j].active = 0;
    }

    gs.num_players = num_clients + 1;

    int player_id = num_clients;

    // Tworzymy inicjalną wiadomość ProtoBuf
    Bomberman__InitialState init_state = BOMBERMAN__INITIAL_STATE__INIT;
    init_state.player_id = player_id;

    // Wypełniamy obiekt mapy
    Bomberman__Map pb_map = BOMBERMAN__MAP__INIT;
    pb_map.width = gs.map.width;
    pb_map.height = gs.map.height;

    // Alokacja tablicy kafelków
    Bomberman__Tile **tiles = malloc(gs.map.width * gs.map.height * sizeof(Bomberman__Tile *));
    if (!tiles)
    {
      perror("Failed to allocate tile array");
      pthread_mutex_unlock(&cl_mutex);
      return;
    }

    int tile_idx = 0;
    for (int y = 0; y < gs.map.height; y++)
    {
      for (int x = 0; x < gs.map.width; x++)
      {
        tiles[tile_idx] = malloc(sizeof(Bomberman__Tile));
        bomberman__tile__init(tiles[tile_idx]);

        tiles[tile_idx]->x = gs.map.tile_arr[y][x].x;
        tiles[tile_idx]->y = gs.map.tile_arr[y][x].y;
        tiles[tile_idx]->type = gs.map.tile_arr[y][x].type;

        tile_idx++;
      }
    }

    pb_map.n_tiles = gs.map.width * gs.map.height;
    pb_map.tiles = tiles;

    init_state.map = &pb_map;

    // Serializacja wiadomości
    size_t len = bomberman__initial_state__get_packed_size(&init_state);
    uint8_t *buffer = malloc(len);

    if (buffer)
    {
      bomberman__initial_state__pack(&init_state, buffer);

      // Wysyłamy wiadomość
      send_protobuf_message(client_socket, buffer, len);

      // Zwalniamy zasoby
      free(buffer);
    }

    // Zwalniamy zasoby związane z tiles
    for (int i = 0; i < pb_map.n_tiles; i++)
    {
      free(tiles[i]);
    }
    free(tiles);

    num_clients++;
  }
  else
  {
    close(client_socket);
  }
  pthread_mutex_unlock(&cl_mutex);
}

void remove_client(int client_socket)
{
  pthread_mutex_lock(&cl_mutex);
  for (int i = 0; i < num_clients; i++)
  {
    if (client_list[i] == client_socket)
    {
      client_list[i] = client_list[num_clients - 1];
      num_clients--;
      break;
    }
  }
  pthread_mutex_unlock(&cl_mutex);
}

void *game_loop(void *arg)
{
  (void)arg; // argument nie jest używany

  while (1)
  {
    // Update bombs and explosions every cycle
    pthread_mutex_lock(&gs_mutex);
    update_bombs(&gs);
    pthread_mutex_unlock(&gs_mutex);

    pthread_mutex_lock(&cl_mutex);

    // Tworzymy aktualny stan gry w ProtoBuf
    Bomberman__GameState pb_state = BOMBERMAN__GAME_STATE__INIT;
    pb_state.num_players = gs.num_players;

    // Tworzymy tablicę graczy
    Bomberman__Player **players = NULL;
    if (gs.num_players > 0)
    {
      players = malloc(gs.num_players * sizeof(Bomberman__Player *));
      if (!players)
      {
        perror("Failed to allocate player array");
        pthread_mutex_unlock(&cl_mutex);
        continue;
      }

      for (int i = 0; i < gs.num_players; i++)
      {
        players[i] = malloc(sizeof(Bomberman__Player));
        bomberman__player__init(players[i]);

        players[i]->id = gs.players[i].id;
        players[i]->pos_x = gs.players[i].pos_x;
        players[i]->pos_y = gs.players[i].pos_y;
        players[i]->alive = gs.players[i].alive;
        players[i]->points = gs.players[i].points;
      }

      pb_state.n_players = gs.num_players;
      pb_state.players = players;
    }

    // Tworzymy mapę
    Bomberman__Map pb_map = BOMBERMAN__MAP__INIT;
    pb_map.width = gs.map.width;
    pb_map.height = gs.map.height;

    // Alokacja tablicy kafelków
    Bomberman__Tile **tiles = malloc(gs.map.width * gs.map.height * sizeof(Bomberman__Tile *));
    if (!tiles)
    {
      perror("Failed to allocate tile array");

      // Zwalniamy zasoby graczy
      if (players)
      {
        for (int i = 0; i < gs.num_players; i++)
        {
          free(players[i]);
        }
        free(players);
      }

      pthread_mutex_unlock(&cl_mutex);
      continue;
    }

    int tile_idx = 0;
    for (int y = 0; y < gs.map.height; y++)
    {
      for (int x = 0; x < gs.map.width; x++)
      {
        tiles[tile_idx] = malloc(sizeof(Bomberman__Tile));
        bomberman__tile__init(tiles[tile_idx]);

        tiles[tile_idx]->x = gs.map.tile_arr[y][x].x;
        tiles[tile_idx]->y = gs.map.tile_arr[y][x].y;
        tiles[tile_idx]->type = gs.map.tile_arr[y][x].type;

        tile_idx++;
      }
    }

    pb_map.n_tiles = gs.map.width * gs.map.height;
    pb_map.tiles = tiles;

    pb_state.map = &pb_map;

    // Serializacja wiadomości
    size_t len = bomberman__game_state__get_packed_size(&pb_state);
    uint8_t *buffer = malloc(len);

    if (buffer)
    {
      bomberman__game_state__pack(&pb_state, buffer);

      // Wysyłamy wiadomość do wszystkich klientów
      for (int i = 0; i < num_clients; i++)
      {
        send_protobuf_message(client_list[i], buffer, len);
      }

      free(buffer);
    }

    // Zwalniamy zasoby
    if (players)
    {
      for (int i = 0; i < gs.num_players; i++)
      {
        free(players[i]);
      }
      free(players);
    }

    for (int i = 0; i < pb_map.n_tiles; i++)
    {
      free(tiles[i]);
    }
    free(tiles);

    pthread_mutex_unlock(&cl_mutex);

    // Tick co około 16ms (~60 FPS)
    usleep(16000);
  }
  return NULL;
}

void *client_handler(void *arg)
{
  int client_socket = *(int *)arg;
  free(arg);

  while (1)
  {
    // Odbieramy wiadomość PlayerMove przez protobuf
    size_t msg_len;
    uint8_t *buffer = recv_protobuf_message(client_socket, &msg_len);

    if (!buffer)
    {
      printf("Klient rozłączony.\n");
      close(client_socket);
      remove_client(client_socket);
      break;
    }

    // Deserializacja wiadomości
    Bomberman__PlayerMove *pb_move = bomberman__player_move__unpack(NULL, msg_len, buffer);
    free(buffer);

    if (pb_move)
    {
      // Konwersja wiadomości protobuf na strukturę PlayerMove
      PlayerMove move;
      move.player_id = pb_move->player_id;
      move.move_type = pb_move->move_type;
      move.pos_x = pb_move->pos_x;
      move.pos_y = pb_move->pos_y;

      // Aktualizacja stanu gry
      pthread_mutex_lock(&gs_mutex);
      update_gamestate(&move, &gs);
      pthread_mutex_unlock(&gs_mutex);

      // Zwalniamy zasoby
      bomberman__player_move__free_unpacked(pb_move, NULL);
    }
  }
  return NULL;
}

int main()
{
  gs.map = initialize_map_from_file("lvl1.txt", 40);

  socklen_t clilen;
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0)
  {
    printf("Couldn't create a server socket. Exiting app...\n");
    exit(1);
  }

  char buffer[BUFFER_SIZE];
  struct sockaddr_in server_addr, client_addr;

  memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    printf("Couldn't bind the socket to the address");
    exit(1);
  }

  listen(server_socket, 5);
  printf("Serwer nasłuchuje na porcie %d...\n", PORT);

  // Utworzenie głównego wątku game_loop
  pthread_t game_thread;
  if (pthread_create(&game_thread, NULL, game_loop, NULL) != 0)
  {
    printf("there was an error creating game loop thread.\n");
    exit(EXIT_FAILURE);
  }

  int *client_socket;
  while (1)
  {
    clilen = sizeof(client_addr);
    client_socket = malloc(sizeof(int));

    if (!client_socket)
    {
      printf("Wystąpił bład w alokacji deskryptoru socketu klienta.\n");
      continue;
    }

    *client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &clilen);
    if (*client_socket < 0)
    {
      printf("Wystąpił błąd w accept.\n");
      continue;
    }

    add_client(*client_socket);

    pthread_t tid;
    if (pthread_create(&tid, NULL, client_handler, client_socket) != 0)
    {
      printf("There was an error creating client thread.\n");
      free(client_socket);
      continue;
    }

    printf("Nowy klient połączony.\n");
    pthread_detach(tid);
  }

  free(client_socket);
  close(server_socket);
  cleanup_map(&gs.map);
  return 0;
}