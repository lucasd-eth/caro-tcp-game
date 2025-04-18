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

void handle_client_turn(int client_socket, int current_player, int game_board[3][3]);
int check_winner(int game_board[3][3]);
void send_state_update(int player1_sock, int player2_sock, int game_board[3][3]);
int server_socket;

int main() {
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);

    // Create TCP socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Prepare server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to address
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    listen(server_socket, 2);
    printf("Server is listening on port %d...\n", PORT);

    while (1) {
        printf("\nĐang chờ 2 người chơi kết nối...\n");

        int client1_sock = accept(server_socket, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len);
        printf("Player 1 connected.\n");
        int client2_sock = accept(server_socket, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len);
        printf("Player 2 connected.\n");

        // Gửi vai trò cho client
        uint8_t role_msg[BUFFER_SIZE] = {0};
        role_msg[0] = 0x05; // ROLE_ASSIGN
        role_msg[1] = PLAYER1;
        send(client1_sock, role_msg, BUFFER_SIZE, 0);

        role_msg[1] = PLAYER2;
        send(client2_sock, role_msg, BUFFER_SIZE, 0);

        // Khởi tạo lại bàn cờ
        int game_board[3][3] = {0};
        int current_player = PLAYER1;
        int move_count = 0;
        uint8_t buffer[BUFFER_SIZE];

        // Game loop
        while (1) {
            int current_sock = (current_player == PLAYER1) ? client1_sock : client2_sock;

            // 1. Gửi TURN_NOTIFICATION
            buffer[0] = 0x01;
            send(current_sock, buffer, BUFFER_SIZE, 0);

            // 2. Nhận nước đi
            handle_client_turn(current_sock, current_player, game_board);
            move_count++;

            // 3. Gửi STATE_UPDATE
            buffer[0] = 0x02;
            int idx = 1;
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    buffer[idx++] = game_board[i][j];
                }
            }
            send(client1_sock, buffer, BUFFER_SIZE, 0);
            send(client2_sock, buffer, BUFFER_SIZE, 0);

            // 4. Kiểm tra kết quả
            int winner = check_winner(game_board);
            if (winner > 0 || move_count == 9) {
                buffer[0] = 0x04; // RESULT
                buffer[1] = (winner > 0) ? winner : 0x03; // DRAW
                send(client1_sock, buffer, BUFFER_SIZE, 0);
                send(client2_sock, buffer, BUFFER_SIZE, 0);

                printf("Ván chơi kết thúc. Kết quả: %s\n", (winner == 1) ? "Player 1 thắng" :
                                                           (winner == 2) ? "Player 2 thắng" : "Hòa");
                close(client1_sock);
                close(client2_sock);
                break;
            }

            // 5. Chuyển lượt
            current_player = (current_player == PLAYER1) ? PLAYER2 : PLAYER1;
        }

        printf("Chuẩn bị cho ván tiếp theo...\n");
    }

    close(server_socket);
    return 0;
}

 void handle_client_turn(int client_sock, int player, int game_board[3][3]) {
    uint8_t buffer[BUFFER_SIZE];
    int valid_move = 0;

    while (!valid_move) {
        recv(client_sock, buffer, BUFFER_SIZE, 0);

        if (buffer[0] == 0x02) { // MOVE
            int row = buffer[1];
            int col = buffer[2];

            if (row >= 0 && row < 3 && col >= 0 && col < 3 && game_board[row][col] == EMPTY) {
                game_board[row][col] = player;
                valid_move = 1; // break the loop
            } else {
                // Gửi lại TURN_NOTIFICATION yêu cầu đánh lại
                buffer[0] = 0x01; // TURN_NOTIFICATION
                send(client_sock, buffer, BUFFER_SIZE, 0);
            }
        }
    }
}


int check_winner(int game_board[3][3]) {
    // Check rows
    for (int i = 0; i < 3; ++i) {
        if (game_board[i][0] == game_board[i][1] && game_board[i][1] == game_board[i][2] && game_board[i][0] != EMPTY) {
            return game_board[i][0];
        }
    }

    // Check columns
    for (int j = 0; j < 3; ++j) {
        if (game_board[0][j] == game_board[1][j] && game_board[1][j] == game_board[2][j] && game_board[0][j] != EMPTY) {
            return game_board[0][j];
        }
    }

    // Check diagonals
    if ((game_board[0][0] == game_board[1][1] && game_board[1][1] == game_board[2][2] && game_board[0][0] != EMPTY) ||
        (game_board[0][2] == game_board[1][1] && game_board[1][1] == game_board[2][0] && game_board[0][2] != EMPTY)) {
        return game_board[1][1];
    }

    return 0;
}

void send_state_update(int player1_sock, int player2_sock, int game_board[3][3]) {
    uint8_t buffer[BUFFER_SIZE];
    buffer[0] = 0x02;

    int idx = 1;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            buffer[idx++] = game_board[i][j];
        }
    }

    send(player1_sock, buffer, BUFFER_SIZE, 0);
    send(player2_sock, buffer, BUFFER_SIZE, 0);
}

