#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 5  // 제한 버퍼 크기
#define NUM_PRODUCERS 2 // 생산자 쓰레드 수
#define NUM_CONSUMERS 2 // 소비자 쓰레드 수

int buffer[BUFFER_SIZE]; // 공유 버퍼
int count = 0;           // 현재 버퍼에 저장된 아이템 수
int in = 0;              // 버퍼에 아이템을 삽입할 위치
int out = 0;             // 버퍼에서 아이템을 제거할 위치

// 동기화 도구
pthread_mutex_t mutex;
pthread_cond_t not_full;
pthread_cond_t not_empty;

// 생산자 쓰레드 함수
void *producer(void *arg) {
    int id = *(int *)arg;
    while (1) {
        int item = rand() % 100; // 생산할 아이템
        pthread_mutex_lock(&mutex);

        // 버퍼가 가득 찬 경우 대기
        while (count == BUFFER_SIZE) {
            pthread_cond_wait(&not_full, &mutex);
        }

        // 아이템을 버퍼에 삽입
        buffer[in] = item;
        in = (in + 1) % BUFFER_SIZE;
        count++;
        printf("Producer %d produced: %d\n", id, item);

        // 소비자에게 알림
        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&mutex);

        sleep(1); // 생산 속도 조절
    }
    return NULL;
}

// 소비자 쓰레드 함수
void *consumer(void *arg) {
    int id = *(int *)arg;
    while (1) {
        pthread_mutex_lock(&mutex);

        // 버퍼가 비어 있는 경우 대기
        while (count == 0) {
            pthread_cond_wait(&not_empty, &mutex);
        }

        // 아이템을 버퍼에서 제거
        int item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;
        printf("Consumer %d consumed: %d\n", id, item);

        // 생산자에게 알림
        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&mutex);

        sleep(1); // 소비 속도 조절
    }
    return NULL;
}

int main() {
    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];
    int producer_ids[NUM_PRODUCERS];
    int consumer_ids[NUM_CONSUMERS];

    // 동기화 객체 초기화
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&not_full, NULL);
    pthread_cond_init(&not_empty, NULL);

    // 생산자 쓰레드 생성
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        producer_ids[i] = i + 1;
        pthread_create(&producers[i], NULL, producer, &producer_ids[i]);
    }

    // 소비자 쓰레드 생성
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        consumer_ids[i] = i + 1;
        pthread_create(&consumers[i], NULL, consumer, &consumer_ids[i]);
    }

    // 쓰레드 종료 대기
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_join(consumers[i], NULL);
    }

    // 동기화 객체 파괴
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&not_full);
    pthread_cond_destroy(&not_empty);

    return 0;
}
