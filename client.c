#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 1024

#define EMPTY 0
#define PLAYER1 1
#define PLAYER2 2

int main() {
    int tcp_sock = 0;
    struct sockaddr_in server_addr;
    uint8_t buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((tcp_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    // Prepare server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    // Connect to server
    if (connect(tcp_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }
    printf("Connected to server.\n");

	int my_role = 0;

    // Nhận vai trò
	recv(tcp_sock, buffer, BUFFER_SIZE, 0);
	if (buffer[0] == 0x05) {
	    my_role = buffer[1];
	    printf("Bạn là người chơi %d\n", my_role);
	}
    // Game loop
     while (1) {
    recv(tcp_sock, buffer, BUFFER_SIZE, 0);

    if (buffer[0] == 0x01) {
        // TURN_NOTIFICATION
        printf("Đến lượt của bạn!\n");

        int row, col;
        printf("Nhập nước đi của bạn (hàng và cột): ");
        scanf("%d %d", &row, &col);

        buffer[0] = 0x02; // MOVE
        buffer[1] = row;
        buffer[2] = col;
        send(tcp_sock, buffer, BUFFER_SIZE, 0);
    }

    else if (buffer[0] == 0x02) {
        // STATE_UPDATE
        printf("Cập nhật bàn cờ:\n");
        int idx = 1;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                printf("%d ", buffer[idx++]);
            }
            printf("\n");
        }
    }

    else if (buffer[0] == 0x04) {
    int result = buffer[1];
    if (result == my_role) {
        printf("Bạn đã thắng!\n");
    } else if (result == 0x03) {
        printf("Trò chơi kết thúc với hòa!\n");
    } else {
        printf("Bạn đã thua!\n");
    }
    break;
    }
}


    // Close socket
    close(tcp_sock);
    return 0;
}

