#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "protocol.h"

int main() {
    int sock_fd;
    struct sockaddr_in server_addr;
    DevicePacket packet;
    int choice;

    // 1. 소켓 생성 및 서버 연결
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DEFAULT_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("서버 연결 실패");
        exit(EXIT_FAILURE);
    }
    printf("서버에 성공적으로 연결되었습니다.\n\n");

    // 2. 무한 루프로 메뉴 출력 및 사용자 입력 받기
    while (1) {
        printf("[ Device Control Menu ]\n");
        printf("1. LED ON\n");
        printf("2. LED OFF\n");
        printf("3. Set Brightness\n");
        printf("4. BUZZER ON (play melody)\n");
        printf("5. BUZZER OFF (stop)\n");
        printf("6. SENSOR ON (감시 시작)\n");
        printf("7. SENSOR OFF (감시 종료)\n");
        printf("8. SEGMENT DISPLAY (숫자 표시 후 카운트다운)\n");
        printf("9. SEGMENT STOP (카운트다운 중단)\n");
        printf("0. Exit\n");
        printf("Select: ");
        
        if (scanf("%d", &choice) != 1) {
            // 숫자가 아닌 값이 입력되었을 때 버퍼를 비워 무한루프 방지
            while (getchar() != '\n');
            printf("올바른 숫자를 입력해주세요.\n\n");
            continue;
        }

        // 패킷에 명령어 담기
        packet.command = choice;
        packet.value = 0; // 기본값

        // 3번 밝기 설정이나 8번 세그먼트처럼 추가 입력이 필요한 경우 처리
        if (choice == 3) {
            printf("밝기 단계를 입력하세요 (1:최저, 2:중간, 3:최대): ");
            scanf("%d", &packet.value);
        } else if (choice == 8) {
            printf("카운트다운 시작할 숫자(0~9)를 입력하세요: ");
            scanf("%d", &packet.value);
        }

        // 0번이면 루프 탈출 후 종료
        if (choice == 0) {
            printf("프로그램을 종료합니다.\n");
            // 서버에도 종료 패킷을 보내 서버측 소켓도 정리하게 해주는 것이 좋습니다.
            send(sock_fd, &packet, sizeof(packet), 0);
            break;
        }

        // 범위를 벗어난 메뉴 선택 예외처리
        if (choice < 0 || choice > 9) {
            printf("잘못된 메뉴 선택입니다. 다시 선택해 주세요.\n\n");
            continue;
        }

        // 3. 서버로 패킷 전송 (선택한 명령어 정보만 전송)
        int send_bytes = send(sock_fd, &packet, sizeof(packet), 0);
        if (send_bytes < 0) {
            perror("데이터 전송 실패");
            break;
        }
        printf(">> 서버로 %d번 명령어 전송 완료.\n", choice);

        // [선택 사항] 서버로부터 작업 완료 응답을 받아와야 화면이 꼬이지 않습니다.
        // char ack_buf[1024];
        // recv(sock_fd, ack_buf, sizeof(ack_buf), 0);
        
        printf("\n"); // 가독성을 위한 줄바꿈
    }

    // 4. 소켓 닫기
    close(sock_fd);
    return 0;
}