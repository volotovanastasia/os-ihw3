#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <server_port>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    // Создание сокета
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Failed to create socket");
        return 1;
    }

    // Настройка адреса сервера
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &(server_address.sin_addr)) <= 0) {
        perror("Invalid server IP address");
        close(socket_fd);
        return 1;
    }

    // Подключение к серверу
    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Failed to connect to server");
        close(socket_fd);
        return 1;
    }

    // Получение данных от сервера
    int round_data[2];
    while (1) {
        int bytes_received = recv(socket_fd, &round_data, sizeof(round_data), 0);
        if (bytes_received == -1) {
            perror("Failed to receive data from server");
            break;
        } else if (bytes_received == 0) {
            printf("Disconnected from server\n");
            break;
        }

        int num_players = round_data[0];
        int winner_number = round_data[1];

        printf("Round over. Number of players: %d. Winner: Player %d\n", num_players, winner_number);
    }

    // Закрытие соединения
    close(socket_fd);

    return 0;
}
