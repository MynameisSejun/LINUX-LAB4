#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define WWW_DIR "./www"
#define CGI_DIR "./cgi-bin"

void handle_sigchld(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// HTTP 요청 구조체
typedef struct {
    char method[16];
    char path[256];
    char query_string[256];
    char protocol[16];
    char content_length[16];
    char content_type[128];
    char body[BUFFER_SIZE];
} http_request;

// HTTP 요청 파싱
void parse_request(char* request_str, http_request* req) {
    char* line = strtok(request_str, "\r\n");
    if (line) {
        sscanf(line, "%s %s %s", req->method, req->path, req->protocol);
        
        // 쿼리 스트링 분리
        char* query = strchr(req->path, '?');
        if (query) {
            *query = '\0';
            strncpy(req->query_string, query + 1, sizeof(req->query_string) - 1);
        }

        // 헤더 파싱
        while ((line = strtok(NULL, "\r\n")) != NULL && strlen(line) > 0) {
            if (strncmp(line, "Content-Length: ", 16) == 0) {
                strncpy(req->content_length, line + 16, sizeof(req->content_length) - 1);
            }
            else if (strncmp(line, "Content-Type: ", 14) == 0) {
                strncpy(req->content_type, line + 14, sizeof(req->content_type) - 1);
            }
        }

        // POST body 파싱
        if (line) {
            line = strtok(NULL, "");
            if (line) {
                strncpy(req->body, line, sizeof(req->body) - 1);
            }
        }
    }
}

// 정적 파일 전송
void send_file(int client_sock, const char* filepath) {
    char buffer[BUFFER_SIZE];
    char response_header[BUFFER_SIZE];
    struct stat st;
    
    if (stat(filepath, &st) < 0) {
        sprintf(response_header, 
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: 22\r\n\r\n"
                "<h1>404 Not Found</h1>");
        write(client_sock, response_header, strlen(response_header));
        return;
    }

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        sprintf(response_header, 
                "HTTP/1.1 500 Internal Server Error\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: 28\r\n\r\n"
                "<h1>500 Server Error</h1>");
        write(client_sock, response_header, strlen(response_header));
        return;
    }

    // Content-Type 결정
    const char* content_type = "text/plain";
    if (strstr(filepath, ".html")) content_type = "text/html";
    else if (strstr(filepath, ".css")) content_type = "text/css";
    else if (strstr(filepath, ".js")) content_type = "application/javascript";

    sprintf(response_header,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n\r\n",
            content_type, st.st_size);
    write(client_sock, response_header, strlen(response_header));

    ssize_t n;
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
        write(client_sock, buffer, n);
    }

    close(fd);
}

// CGI 프로그램 실행
void execute_cgi(int client_sock, http_request* req, const char* script_path) {
     printf("Executing CGI: %s\n", script_path);  // 디버그 출력 추가

    // 파일 존재 여부 확인
    if (access(script_path, X_OK) != 0) {
        printf("CGI script not executable or not found: %s (errno: %d)\n", 
               script_path, errno);
        return;
    }
    
    char buffer[BUFFER_SIZE];
    int pipe_in[2], pipe_out[2];

    if (pipe(pipe_in) < 0 || pipe(pipe_out) < 0) {
        perror("pipe");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {  // 자식 프로세스
        close(pipe_in[1]);
        close(pipe_out[0]);
        dup2(pipe_in[0], STDIN_FILENO);
        dup2(pipe_out[1], STDOUT_FILENO);

        // 환경 변수 설정
        setenv("REQUEST_METHOD", req->method, 1);
        setenv("QUERY_STRING", req->query_string, 1);
        if (strcmp(req->method, "POST") == 0) {
            setenv("CONTENT_LENGTH", req->content_length, 1);
            setenv("CONTENT_TYPE", req->content_type, 1);
        }

        execl(script_path, script_path, NULL);
        exit(1);
    }

    // 부모 프로세스
    close(pipe_in[0]);
    close(pipe_out[1]);

    // POST 데이터 전송
    if (strcmp(req->method, "POST") == 0) {
        write(pipe_in[1], req->body, strlen(req->body));
    }
    close(pipe_in[1]);

    // CGI 출력 읽기
    ssize_t n;
    int header_sent = 0;
    while ((n = read(pipe_out[0], buffer, sizeof(buffer))) > 0) {
        if (!header_sent) {
            // CGI 스크립트가 헤더를 포함하지 않은 경우 기본 헤더 추가
            if (!strstr(buffer, "Content-Type")) {
                char header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
                write(client_sock, header, strlen(header));
            }
            header_sent = 1;
        }
        write(client_sock, buffer, n);
    }

    close(pipe_out[0]);
    waitpid(pid, NULL, 0);
}

void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE];
    ssize_t n = read(client_sock, buffer, sizeof(buffer) - 1);
    if (n <= 0) return;

    buffer[n] = '\0';
    http_request req = {0};
    parse_request(buffer, &req);

    char filepath[512];
    // CGI 요청 처리
    if (strncmp(req.path, "/cgi-bin/", 9) == 0) {
        snprintf(filepath, sizeof(filepath), "%s/%s", CGI_DIR, req.path + 9);
        execute_cgi(client_sock, &req, filepath);
    }
    // 정적 파일 처리
    else {
        if (strcmp(req.path, "/") == 0) {
            strcpy(req.path, "/index.html");
        }
        snprintf(filepath, sizeof(filepath), "%s/%s", WWW_DIR, req.path + 1);
        send_file(client_sock, filepath);
    }
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // SIGCHLD 핸들러 설정
    signal(SIGCHLD, handle_sigchld);

    // 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(1);
    }

    // SO_REUSEADDR 설정
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    // 바인드
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    // 리스닝
    if (listen(server_sock, 5) < 0) {
        perror("listen");
        exit(1);
    }

    printf("서버가 포트 %d에서 실행 중입니다...\n", PORT);

    // 클라이언트 요청 처리
    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }

        // 자식 프로세스 생성하여 요청 처리
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(client_sock);
            continue;
        }

        if (pid == 0) {  // 자식 프로세스
            close(server_sock);
            handle_client(client_sock);
            close(client_sock);
            exit(0);
        }
        else {  // 부모 프로세스
            close(client_sock);
        }
    }

    close(server_sock);
    return 0;
}