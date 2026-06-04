#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include "../include/protocol.h"

// 메뉴판을 깔끔하게 띄우기 위한 함수
void print_menu() {
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
    fflush(stdout);
}

int main() {
    int sock_fd;
    struct sockaddr_in server_addr;
    DevicePacket packet;
    int choice;
    int sensor_mode = 0; // 센서 감시가 켜져있는지 확인하는 플래그

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DEFAULT_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("서버 연결 실패");
        exit(EXIT_FAILURE);
    }
    printf("서버에 성공적으로 연결되었습니다.\n");
    
    // 최초 메뉴판 출력
    print_menu();

    // ★ 클라이언트 메인 루프를 통째로 select() 비동기 루프로 만듭니다.
    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds); // 키보드 감시
        FD_SET(sock_fd, &read_fds);      // 서버 패킷 감시
        
        // 서버 데이터나 키보드 입력이 올 때까지 대기
        if (select(sock_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select 에러");
            break;
        }

        // ----------------------------------------------------
        // 1. 서버에서 패킷(센서 데이터 등)이 날아온 경우
        // ----------------------------------------------------
        if (FD_ISSET(sock_fd, &read_fds)) {
            SensorResponsePacket res;
            int recv_bytes = recv(sock_fd, &res, sizeof(res), 0);
            
            if (recv_bytes <= 0) {
                printf("\n[오류] 서버와 연결이 끊어졌습니다.\n");
                break;
            }
            
            // 센서 데이터 수신 처리
            if (res.type == RESP_SENSOR_DATA) {
                // \r을 이용해 현재 줄을 덮어쓰면서 프롬프트를 유지하는 UI 해킹 기법
                if (res.light_detected == 1) {
                    printf("\r[SENSOR] ☀️ 빛 감지됨! (LED ON)   | 명령어 입력: ");
                } else {
                    printf("\r[SENSOR] 🌑 어두움 (LED OFF)      | 명령어 입력: ");
                }
                fflush(stdout); 
            }
        }
        
        // ----------------------------------------------------
        // 2. 사용자가 키보드로 숫자를 입력한 경우
        // ----------------------------------------------------
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (scanf("%d", &choice) == 1) {
                packet.command = choice;
                packet.value = 0;

                // 추가 입력이 필요한 명령어 처리
                if (choice == 3) {
                    printf("밝기 단계를 입력하세요 (1:최저, 2:중간, 3:최대): ");
                    scanf("%d", &packet.value);
                } else if (choice == 8) {
                    printf("시작할 숫자(0~9)를 입력하세요: ");
                    scanf("%d", &packet.value);
                }

                // 0번 종료 처리
                if (choice == 0) {
                    printf("\n프로그램을 종료합니다.\n");
                    send(sock_fd, &packet, sizeof(packet), 0);
                    break;
                }
                
                // 센서 UI 상태 업데이트
                if (choice == 6) sensor_mode = 1;
                if (choice == 7) sensor_mode = 0;

                // 유효한 명령어 전송
                if (choice >= 1 && choice <= 9) {
                    send(sock_fd, &packet, sizeof(packet), 0);
                    
                    // 센서 감시 중이 아니라면, 전송 완료 후 메뉴판을 다시 예쁘게 그려줍니다.
                    if (!sensor_mode) {
                        printf(">> %d번 명령어 전송 완료.\n", choice);
                        print_menu(); 
                    }
                } else {
                    printf("잘못된 입력입니다.\n");
                    if (!sensor_mode) print_menu();
                }

            } else {
                // 숫자가 아닌 글자 입력 시 버퍼 청소
                while(getchar() != '\n');
                printf("숫자만 입력해주세요.\n");
                if (!sensor_mode) print_menu();
            }
        }
    }

    close(sock_fd);
    return 0;
}