#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define MAX_CLIENTS 10 // 최대 클라이언트 수
#define BUFFER_SIZE 1024
#define PORT 8080

int main() {
    int server_socket, client_socket, max_fd, activity, new_socket;
    int client_sockets[MAX_CLIENTS] = {0};
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    fd_set read_fds;

    // 서버 소켓 생성
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 구조체 초기화
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 소켓과 포트 바인딩
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // 리스닝 시작
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Chat server started on port %d\n", PORT);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);
        max_fd = server_socket;

        // 기존 클라이언트를 FD_SET에 추가
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0) {
                FD_SET(client_sockets[i], &read_fds);
            }
            if (client_sockets[i] > max_fd) {
                max_fd = client_sockets[i];
            }
        }

        // 소켓 상태 확인
        activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Select error");
            continue;
        }

        // 새로운 연결 요청 처리
        if (FD_ISSET(server_socket, &read_fds)) {
            if ((new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len)) < 0) {
                perror("Accept failed");
                continue;
            }

            printf("New connection: socket %d, IP %s, PORT %d\n",
                   new_socket, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            // 새로운 클라이언트를 배열에 추가
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    printf("Adding client to list at index %d\n", i);
                    break;
                }
            }
        }

        // 기존 클라이언트 소켓의 데이터 읽기
        for (int i = 0; i < MAX_CLIENTS; i++) {
            client_socket = client_sockets[i];

            if (FD_ISSET(client_socket, &read_fds)) {
                int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
                if (bytes_read == 0) {
                    // 연결 종료
                    printf("Client disconnected: socket %d\n", client_socket);
                    close(client_socket);
                    client_sockets[i] = 0;
                } else {
                    // 메시지 브로드캐스트
                    buffer[bytes_read] = '\0';
                    printf("Message from client %d: %s", client_socket, buffer);

                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (client_sockets[j] != 0 && client_sockets[j] != client_socket) {
                            send(client_sockets[j], buffer, bytes_read, 0);
                        }
                    }
                }
            }
        }
    }

    close(server_socket);
    return 0;
}
