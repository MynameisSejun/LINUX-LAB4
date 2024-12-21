#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // 환경 변수에서 POST 데이터의 길이를 가져옴
    char* len_ = getenv("CONTENT_LENGTH");
    int len = atoi(len_);  // 길이를 정수로 변환

    char data[1024];  // POST 데이터를 저장할 배열
    fgets(data, len + 1, stdin);  // POST 데이터 읽기

    // HTTP 헤더 출력
    printf("Content-Type: text/plain\n\n");

    // POST 데이터 파싱
    char* name = strstr(data, "name=");  // "name=" 시작 위치 찾기
    char* message = strstr(data, "&message=");  // "&message=" 시작 위치 찾기

    if (name && message) {
        name += 5;  // "name=" 다음 문자로 포인터 이동
        *message = '\0';  // "name" 파라미터의 끝에 NULL 문자 삽입
        message += 9;  // "&message=" 다음 문자로 포인터 이동

        // 이름과 메시지 출력
        printf("Name: %s\n", name);
        printf("Message: %s\n", message);
    } else {
        // 데이터 파싱 실패 시 오류 메시지 출력
        printf("Error parsing input data.\n");
    }

    return 0;
}
