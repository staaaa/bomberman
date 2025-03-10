#include "../common/gamestate.h"
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

void *receive_data(void *arg);
void *send_data(void *arg);

// address is first, port is second
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Nie podano adresu ip albo portu, konczenie programu...");
        exit(1);
    }

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    int bytes_recived;
    GameState *buffer = malloc(sizeof(GameState));
    struct sockaddr_in server_addr;

    // if we are opening client on the same machine as server then pass 0 as server ip
    if (*argv[1] == '0')
    {
        server_addr.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0)
        {
            perror("Błąd konwersji adresu IP");
            close(client_socket);
            exit(EXIT_FAILURE);
        }
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("wystąpił błąd połączenia z serwerem...");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Połączono z serwerem.");
    pthread_t recv_thread, send_thread;
    pthread_create(&recv_thread, NULL, receive_data, &client_socket);
    pthread_create(&send_thread, NULL, send_data, &client_socket);

    pthread_join(recv_thread, NULL);
    pthread_join(send_thread, NULL);
    close(client_socket);
    return 0;
}
