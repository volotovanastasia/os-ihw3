#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

// Структура для хранения данных клиента
typedef struct {
    int socket;
    int number;
    int client_number;
} ClientData;

// Функция для обработки клиентского подключения
void handle_client(ClientData *client)
{
    printf("Received number (energy) %d from client %d\n", client->number, client->client_number);
    send(client->socket, &client->client_number, sizeof(client->client_number), 0);
    close(client->socket);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Usage: %s <port> <num_clients>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int num_clients = atoi(argv[2]);

    int server_socket;
    struct sockaddr_in server_address;

    // Создание сокета
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Failed to create socket");
        return 1;
    }

    // Настройка адреса сервера
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    // Привязка сокета к адресу
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Failed to bind");
        close(server_socket);
        return 1;
    }

    // Ожидание подключений
    if (listen(server_socket, num_clients) == -1) {
        perror("Failed to listen");
        close(server_socket);
        return 1;
    }

    printf("Server started. IP address is %s\n Waiting for %d clients...\n", inet_ntoa(server_address.sin_addr), num_clients);

    ClientData clients[num_clients];
    int connected_clients = 0;

    while (connected_clients < num_clients) {
        // Принятие подключения от клиента
        struct sockaddr_in client_address;
        int client_address_length = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address,
                                   (socklen_t *)&client_address_length);

        if (client_socket == -1) {
            perror("Failed to accept connection");
            continue;
        }

        printf("Client %d connected\n", connected_clients + 1);

        // Получение числа от клиента
        int number;
        int bytes_received = recv(client_socket, &number, sizeof(number), 0);
        if (bytes_received == -1) {
            perror("Failed to receive data from client");
            close(client_socket);
            continue;
        }

        // Сохранение данных клиента
        clients[connected_clients].socket = client_socket;
        clients[connected_clients].number = number;
        clients[connected_clients].client_number = connected_clients + 1;

        // Обработка клиентского подключения в отдельном потоке
        handle_client(&clients[connected_clients]);

        connected_clients++;
    }

    printf("All clients connected. Processing data...\n");

    // Обработка данных
    int current_round[num_clients];
    for (int i = 0; i < num_clients; i++) {
        current_round[i] = clients[i].client_number;
    }

    // Проведение соревнований
    srand(time(NULL));
    int new_num_clients = num_clients / 2 + num_clients % 2;
    while (new_num_clients > 1) {
        int next_round[new_num_clients];
        for (int i = 0; i < new_num_clients; i++) {
            if (i == new_num_clients - 1 && num_clients % 2 == 1) {
                next_round[i] = current_round[i * 2];
                clients[current_round[i * 2]].number = clients[current_round[i * 2]].number * 2;
            } else {
                int random = rand() % 2;
                next_round[i] = current_round[i * 2 + random];
                clients[current_round[i * 2 + random]].number = clients[current_round[i * 2 + (random + 1) % 2]].number;
            }
        }
        for (int i = 0; i < new_num_clients; i++) {
            current_round[i] = next_round[i];
        }
        new_num_clients = new_num_clients / 2 + new_num_clients % 2;
        printf("The round is over. The number of participants has been reduced to %d\n", new_num_clients);
    }

    printf("The participant %d won with energy %d\n", clients[current_round[0]].client_number, clients[current_round[0]].number);

    // Завершение работы сервера
    close(server_socket);

    return 0;
}
