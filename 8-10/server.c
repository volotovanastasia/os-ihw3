#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>

#define MAX_PLAYERS 100
#define MAX_OBSERVERS 100
#define BUFFER_SIZE 1024

// Структура для хранения данных клиента
typedef struct {
    int socket;
    int number;
    int client_number;
} ClientData;

// Функция для отправки сообщения наблюдателю
void send_message_to_observer(int observer_socket, const char* message)
{
    send(observer_socket, message, strlen(message), 0);
}

// Функция для обработки клиентского подключения
void handle_client(ClientData *client, int observer_sockets[], int num_observers)
{
    char message[BUFFER_SIZE];
    snprintf(message, BUFFER_SIZE, "Received number (energy) %d from client %d\n", client->number, client->client_number);

    // Отправка сообщения наблюдателям
    for (int i = 0; i < num_observers; i++) {
        send_message_to_observer(observer_sockets[i], message);
    }

    send(client->socket, &client->client_number, sizeof(client->client_number), 0);
}

// Функция для обработки подключения наблюдателя
void handle_observer(int observer_socket)
{
    // Отправь сообщение наблюдателю
    char message[BUFFER_SIZE];
    snprintf(message, BUFFER_SIZE, "Welcome, observer!\n");
    send_message_to_observer(observer_socket, message);
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        printf("Usage: %s <players_port> <observers_port> <num_players>\n", argv[0]);
        return 1;
    }

    int players_port = atoi(argv[1]);
    int observers_port = atoi(argv[2]);
    int num_players = atoi(argv[3]);

    int server_socket_players;
    int server_socket_observers;
    struct sockaddr_in server_address_players;
    struct sockaddr_in server_address_observers;

    // Создание сокета для игроков
    server_socket_players = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_players == -1) {
        perror("Failed to create players socket");
        return 1;
    }

    // Создание сокета для наблюдателей
    server_socket_observers = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_observers == -1) {
        perror("Failed to create observers socket");
        close(server_socket_players);
        return 1;
    }

    // Настройка адреса сервера для игроков
    server_address_players.sin_family = AF_INET;
    server_address_players.sin_addr.s_addr = INADDR_ANY;
    server_address_players.sin_port = htons(players_port);

    // Настройка адреса сервера для наблюдателей
    server_address_observers.sin_family = AF_INET;
    server_address_observers.sin_addr.s_addr = INADDR_ANY;
    server_address_observers.sin_port = htons(observers_port);

    // Привязка сокета игроков к адресу
    if (bind(server_socket_players, (struct sockaddr *)&server_address_players, sizeof(server_address_players)) == -1) {
        perror("Failed to bind players socket");
        close(server_socket_players);
        close(server_socket_observers);
        return 1;
    }

    // Привязка сокета наблюдателей к адресу
    if (bind(server_socket_observers, (struct sockaddr *)&server_address_observers, sizeof(server_address_observers)) == -1) {
        perror("Failed to bind observers socket");
        close(server_socket_players);
        close(server_socket_observers);
        return 1;
    }

    // Ожидание подключений от игроков
    if (listen(server_socket_players, num_players) == -1) {
        perror("Failed to listen for players");
        close(server_socket_players);
        close(server_socket_observers);
        return 1;
    }

    // Ожидание подключений от наблюдателей
    if (listen(server_socket_observers, MAX_OBSERVERS) == -1) {
        perror("Failed to listen for observers");
        close(server_socket_players);
        close(server_socket_observers);
        return 1;
    }

    printf("Server started. Players IP address is %s. Observers IP address is %s\n", inet_ntoa(server_address_players.sin_addr), inet_ntoa(server_address_observers.sin_addr));
    printf("Waiting for %d players...\n", num_players);

    ClientData clients[MAX_PLAYERS];
    int connected_players = 0;
    int observer_sockets[MAX_OBSERVERS];
    int num_observers = 0;

    while (connected_players < num_players) {
        // Принятие подключения от игрока
        struct sockaddr_in player_address;
        socklen_t player_address_length = sizeof(player_address);
        int player_socket = accept(server_socket_players, (struct sockaddr *)&player_address, &player_address_length);

        if (player_socket == -1) {
            perror("Failed to accept player connection");
            continue;
        }

        printf("Player %d connected\n", connected_players + 1);

        // Получение числа от игрока
        int number;
        int bytes_received = recv(player_socket, &number, sizeof(number), 0);
        if (bytes_received == -1) {
            perror("Failed to receive data from player");
            close(player_socket);
            continue;
        }

        // Сохранение данных игрока
        clients[connected_players].socket = player_socket;
        clients[connected_players].number = number;
        clients[connected_players].client_number = connected_players + 1;

        // Обработка клиентского подключения в отдельном потоке
        handle_client(&clients[connected_players], observer_sockets, num_observers);

        connected_players++;
    }

    printf("All players connected. Processing data...\n");

    // Ожидание подключения наблюдателей
    while (num_observers < MAX_OBSERVERS) {
        struct sockaddr_in observer_address;
        socklen_t observer_address_length = sizeof(observer_address);
        int observer_socket = accept(server_socket_observers, (struct sockaddr *)&observer_address, &observer_address_length);

        if (observer_socket == -1) {
            perror("Failed to accept observer connection");
            break;
        }

        // Обработка подключения наблюдателя
        handle_observer(observer_socket);

        // Сохранение сокета наблюдателя
        observer_sockets[num_observers] = observer_socket;
        num_observers++;
    }

    int current_round[num_players];
    for (int i = 0; i < num_players; i++) {
        current_round[i] = clients[i].client_number;
    }

    srand(time(NULL));
    int new_num_players = num_players / 2 + num_players % 2;
    while (new_num_players > 1) {
        int next_round[new_num_players];
        for (int i = 0; i < new_num_players; i++) {
            if (i == new_num_players - 1 && new_num_players % 2 == 1) {
                next_round[i] = current_round[i * 2];
                clients[current_round[i * 2]].number = clients[current_round[i * 2]].number * 2;
            } else {
                int random = rand() % 2;
                next_round[i] = current_round[i * 2 + random];
                clients[current_round[i * 2 + random]].number = clients[current_round[i * 2 + (random + 1) % 2]].number;
                int loser_message = current_round[i * 2 + random];
                for (int j = 0; j < num_observers; j++) {
                    send(observer_sockets[j], &loser_message, sizeof(loser_message), 0);
                }
                close(clients[current_round[i * 2 + (random + 1) % 2]].socket);
            }
        }
        for (int i = 0; i < new_num_players; i++) {
            current_round[i] = next_round[i];
        }
        new_num_players = new_num_players / 2 + new_num_players % 2;
        printf("The round is over. The number of participants has been reduced to %d\n", new_num_players);
        // Отправка сообщения наблюдателям о завершении раунда и количестве участников
        int observer_message[2] = {new_num_players, current_round[0]};
        for (int i = 0; i < num_observers; i++) {
            send(observer_sockets[i], &observer_message, sizeof(observer_message), 0);
        }
        sleep(10);  // Задержка на 10 секунд
    }

    printf("The participant %d won with energy %d\n", clients[current_round[0]].client_number, clients[current_round[0]].number);
    for (int i = 0; i < num_observers; i++) {
        send(observer_sockets[i], &current_round[0], sizeof(current_round[0]), 0);
        close(observer_sockets[i]);
    }
    send(clients[current_round[0]].socket, &clients[current_round[0]].client_number, sizeof(clients[current_round[0]].client_number), 0);
    close(clients[current_round[0]].socket);


    for (int i = 0; i < num_observers; i++) {
        close(observer_sockets[i]);
    }

    // Завершение работы сервера
    close(server_socket_players);
    close(server_socket_observers);

    return 0;
}
