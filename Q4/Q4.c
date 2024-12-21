#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_CLIENTS 3        // 클라이언트 쓰레드 수
#define MAX_MESSAGE_LENGTH 256 // 메시지 최대 길이

// 동기화 변수
pthread_mutex_t mutex;
pthread_cond_t broadcast_cond;
pthread_cond_t input_cond;
int current_input_client = 0; // 현재 입력을 받을 클라이언트 ID

// 공유 데이터
char message[MAX_MESSAGE_LENGTH]; // 방송 메시지
int has_message = 0;              // 메시지 존재 여부

// 클라이언트 쓰레드 함수
void *client_thread(void *arg) {
    int id = *(int *)arg;

    while (1) {
        char client_message[MAX_MESSAGE_LENGTH];

        // 자신의 차례가 될 때까지 대기
        pthread_mutex_lock(&mutex);
        while (current_input_client != id) {
            pthread_cond_wait(&input_cond, &mutex);
        }

        // 사용자 입력을 통한 메시지 생성
        printf("[Client %d] Enter your message: ", id);
        fflush(stdout); // 출력 버퍼 강제 비우기
        fgets(client_message, sizeof(client_message), stdin);

        // 줄바꿈 문자 제거
        client_message[strcspn(client_message, "\n")] = '\0';

        // 메시지가 이미 있으면 대기
        while (has_message) {
            pthread_cond_wait(&broadcast_cond, &mutex);
        }

        // 메시지를 공유 버퍼에 저장
        snprintf(message, sizeof(message), "Client %d: %s", id, client_message);
        printf("[Client %d] Sending message: %s\n", id, client_message);
        has_message = 1;

        // 다음 클라이언트로 입력 순서 이동
        current_input_client = (current_input_client % NUM_CLIENTS) + 1;

        // 서버에게 메시지 준비 완료 알림
        pthread_cond_signal(&broadcast_cond);
        pthread_cond_broadcast(&input_cond);
        pthread_mutex_unlock(&mutex);

        sleep(1); // 메시지 전송 간격 조절
    }
    return NULL;
}

// 서버 쓰레드 함수
void *server_thread(void *arg) {
    (void)arg; // 인자가 필요하지 않음

    while (1) {
        pthread_mutex_lock(&mutex);

        // 메시지가 없으면 대기
        while (!has_message) {
            pthread_cond_wait(&broadcast_cond, &mutex);
        }

        // 모든 클라이언트에게 메시지 방송
        printf("[Server] Broadcasting message: %s\n", message);
        has_message = 0;

        // 클라이언트 쓰레드에 알림
        pthread_cond_broadcast(&broadcast_cond);
        pthread_mutex_unlock(&mutex);

        sleep(1); // 방송 간격 조절
    }
    return NULL;
}

int main() {
    pthread_t server;
    pthread_t clients[NUM_CLIENTS];
    int client_ids[NUM_CLIENTS];

    // 동기화 변수 초기화
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&broadcast_cond, NULL);
    pthread_cond_init(&input_cond, NULL);

    // 서버 쓰레드 생성
    pthread_create(&server, NULL, server_thread, NULL);

    // 클라이언트 쓰레드 생성
    for (int i = 0; i < NUM_CLIENTS; i++) {
        client_ids[i] = i + 1;
        pthread_create(&clients[i], NULL, client_thread, &client_ids[i]);
    }

    // 입력 순서를 첫 번째 클라이언트로 설정
    pthread_mutex_lock(&mutex);
    current_input_client = 1;
    pthread_cond_broadcast(&input_cond);
    pthread_mutex_unlock(&mutex);

    // 쓰레드 종료 대기
    pthread_join(server, NULL);
    for (int i = 0; i < NUM_CLIENTS; i++) {
        pthread_join(clients[i], NULL);
    }

    // 동기화 변수 해제
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&broadcast_cond);
    pthread_cond_destroy(&input_cond);

    return 0;
}
