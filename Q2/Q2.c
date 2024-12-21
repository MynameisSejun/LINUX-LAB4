#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// 쓰레드 함수 1: 1~10까지 숫자를 출력
void *print_numbers(void *arg) {
    int id = *(int *)arg;
    printf("Thread %d: Start printing numbers\n", id);
    for (int i = 1; i <= 10; i++) {
        printf("Thread %d: %d\n", id, i);
        sleep(1); // 1초 대기
    }
    printf("Thread %d: Done printing numbers\n", id);
    return NULL;
}

// 쓰레드 함수 2: 10~1까지 숫자를 출력
void *print_reverse_numbers(void *arg) {
    int id = *(int *)arg;
    printf("Thread %d: Start printing reverse numbers\n", id);
    for (int i = 10; i >= 1; i--) {
        printf("Thread %d: %d\n", id, i);
        sleep(1); // 1초 대기
    }
    printf("Thread %d: Done printing reverse numbers\n", id);
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    int thread1_id = 1, thread2_id = 2;

    // 쓰레드 생성
    if (pthread_create(&thread1, NULL, print_numbers, &thread1_id) != 0) {
        perror("Failed to create thread1");
        return 1;
    }

    if (pthread_create(&thread2, NULL, print_reverse_numbers, &thread2_id) != 0) {
        perror("Failed to create thread2");
        return 1;
    }

    // 쓰레드 종료 대기
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("Main thread: All threads are finished.\n");
    return 0;
}
