#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    if (argc != 4) {
        printf("Usage: %s <server_ip> <server_port> <number>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int number = atoi(argv[3]);

    int client_socket;
    struct sockaddr_in server_address;

// Создание сокета
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Failed to create socket");
        return 1;
    }

// Настройка адреса сервера
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &(server_address.sin_addr)) <= 0) {
        perror("Failed to convert IP address");
        close(client_socket);
        return 1;
    }

// Подключение к серверу
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Failed to connect to server");
        close(client_socket);
        return 1;
    }

    printf("Connected to the server\n");

// Отправка числа серверу
    ssize_t bytes_sent = send(client_socket, &number, sizeof(number), 0);
    if (bytes_sent == -1) {
        perror("Failed to send data to server");
        close(client_socket);
        return 1;
    } else if (bytes_sent != sizeof(number)) {
        fprintf(stderr, "Failed to send complete data to server\n");
        close(client_socket);
        return 1;
    }

// Получение порядкового номера от сервера
    int client_number;
    ssize_t bytes_received = recv(client_socket, &client_number, sizeof(client_number), 0);
    if (bytes_received == -1) {
        perror("Failed to receive data from server");
        close(client_socket);
        return 1;
    } else if (bytes_received != sizeof(client_number)) {
        fprintf(stderr, "Failed to receive complete data from server\n");
        close(client_socket);
        return 1;
    }

    printf("Received client number: %d\n", client_number);

// Получение номера победителя от сервера
    int winner_number;
    bytes_received = recv(client_socket, &winner_number, sizeof(winner_number), 0);
    if (bytes_received == -1) {
        perror("Failed to receive data from server");
        close(client_socket);
        return 1;
    } else if (bytes_received != sizeof(winner_number)) {
        fprintf(stderr, "Failed to receive complete data from server\n");
        close(client_socket);
        return 1;
    }

// Проверка на выигрыш или проигрыш
    if (winner_number == client_number) {
        printf("You won!\n");
    } else {
        printf("You lost. The winner is client number: %d\n", winner_number);
    }

// Завершение работы клиента
    close(client_socket);

    return 0;
}
