#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include "../include/protocol.h"
#include <fcntl.h> // ★ open 함수 사용을 위해 추가

// 전역 변수로 서버 소켓 핸들 선언 (시그널 핸들러에서 닫기 위함)
int server_fd = -1;

void* device_command_thread(void* arg);

void myThreadCreate(int command, int value, int client_fd) {
    // 1. 스레드에게 줄 인자 메모리를 동적 할당 (malloc)
    // ※ 그냥 지역변수로 주소를 넘기면, while루프가 돌면서 값이 바뀔 수 있어 위험합니다.
    ThreadArgs* args = (ThreadArgs*)malloc(sizeof(ThreadArgs));
    args->command = command;
    args->value = value;
    args->client_fd = client_fd;

    // 2. 스레드 생성
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, device_command_thread, (void*)args) != 0) {
        perror("[스레드 에러] 스레드 생성 실패");
        free(args); // 생성 실패 시 메모리 해제
    } else {
        // 3. 스레드 분리 (Detach)
        // 스레드가 종료되면 OS가 자원을 알아서 즉시 회수하도록 설정합니다. (pthread_join 생략 가능)
        pthread_detach(thread_id);
    }
}

// Ctrl+C (인터럽트) 발생 시 안전하게 소켓을 닫고 종료하기 위한 시그널 핸들러
void handle_sigint(int sig) {
    (void)sig; // 사용하지 않는 매개변수 경고 방지
    printf("\n[알림] 인터럽트 신호(SIGINT) 감지. 서버를 종료합니다.\n");
    if (server_fd >= 0) {
        close(server_fd);
    }
    exit(EXIT_SUCCESS);
}

int main() {
    // =========================================================================
    // ★ [데몬화 로직 추가] ★
    // 1. daemon(1, 0) 호출
    // - 첫 번째 인자(1): 현재 작업 디렉토리 유지 (0이면 "/"로 가서 dlopen 상대경로가 다 고장남)
    // - 두 번째 인자(0): 표준 입출력(화면)을 닫고 /dev/null로 보냄
    if (daemon(1, 0) == -1) {
        perror("데몬 프로세스 전환 실패");
        exit(EXIT_FAILURE);
    }

    // 2. 화면 출력이 끊겼으므로, printf 로그를 파일로 남기도록 방향 틀기 (Redirection)
    int log_fd = open("server_log.txt", O_RDWR | O_CREAT | O_APPEND, 0644);
    if (log_fd != -1) {
        dup2(log_fd, STDOUT_FILENO); // 기존 화면 출력(printf)을 파일로 연결
        dup2(log_fd, STDERR_FILENO); // 기존 에러 출력(perror)을 파일로 연결
        close(log_fd);
        
        // 파일에 즉시 쓰이도록 버퍼링 비활성화
        setvbuf(stdout, NULL, _IONBF, 0); 
        setvbuf(stderr, NULL, _IONBF, 0);
    }

    int client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int opt = 1;

    // 1. 시그널 핸들러 등록 (인터럽트 전까지 무한 실행 보장)
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint); // ★ 데몬은 kill 명령(SIGTERM)으로 죽이므로 이것도 추가!
    signal(SIGPIPE, SIG_IGN);

    // 2. 서버 리슨 소켓 생성
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("소켓 생성 실패");
        exit(EXIT_FAILURE);
    }

    // 포트 재사용 옵션 설정
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt 실패");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 3. 주소 및 포트 바인딩
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(DEFAULT_PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("바인딩 실패");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 4. 접속 대기열 생성
    if (listen(server_fd, 5) < 0) {
        perror("리스닝 실패");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("=========================================\n");
    printf(" TCP 원격 장치 제어 서버 구동 중... (Port: %d)\n", DEFAULT_PORT);
    printf(" [종료하려면 Ctrl+C를 누르세요]\n");
    printf("=========================================\n");

    // [서버 메인 루프] 인터럽트가 오기 전까지 대기 및 수신을 반복합니다.
    while (1) {
        printf("\n[대기] 클라이언트의 접속을 기다리는 중...\n");
        
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_fd < 0) {
            // 인터럽트로 종료될 때 accept에서 에러가 날 수 있으므로 체크
            perror("접속 수락 실패");
            continue; 
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("[연결] 클라이언트 접속 완료! (IP: %s, Port: %d)\n", client_ip, ntohs(client_addr.sin_port));

        // [데이터 수신 루프] 한 클라이언트가 연결된 동안 패킷을 계속 받습니다.
        while (1) {
            DevicePacket packet;
            
            // 구조체 크기(fixed-size)만큼 데이터를 통째로 수신
            int recv_bytes = recv(client_fd, &packet, sizeof(DevicePacket), 0);
            
            if (recv_bytes > 0) {
                // 정상 수신: 콘솔에 로그 출력
                printf("[로그 수신] Command: %d, Value: %d -> ", packet.command, packet.value);
                
                // 명령어 종류에 따른 상세 텍스트 매핑 로그
                switch (packet.command) {
                    case CMD_EXIT:
                        printf("클라이언트가 종료(Exit)를 요청했습니다.\n");
                        break;
                    case CMD_LED_ON:
                        printf("LED 켜기 명령\n");
                        myThreadCreate(packet.command, packet.value, client_fd);
                        break;
                    case CMD_LED_OFF:
                        printf("LED 끄기 명령\n");
                        myThreadCreate(packet.command, packet.value, client_fd);
                        break;
                    case CMD_SET_BRIGHT:
                        printf("LED 밝기 설정 명령 (밝기값: %d)\n", packet.value);
                        myThreadCreate(packet.command, packet.value, client_fd);
                        break;
                    case CMD_BUZZER_ON:
                        printf("부저 켜기 멜로디 재생 명령\n");
                        myThreadCreate(packet.command, packet.value, client_fd);
                        break;
                    case CMD_BUZZER_OFF:
                        printf("부저 끄기 명령\n");
                        myThreadCreate(packet.command, packet.value, client_fd);
                        break;
                    case CMD_SENSOR_ON:
                        printf("조도센서 감시 시작 명령\n");
                        myThreadCreate(packet.command, packet.value, client_fd);
                        break;
                    case CMD_SENSOR_OFF:
                        printf("조도센서 감시 종료 명령\n");
                        myThreadCreate(packet.command, packet.value, client_fd);
                        break;
                    case CMD_SEGMENT_DISP:
                        printf("7세그먼트 카운트다운 표시 명령 (시작숫자: %d)\n", packet.value);
                        myThreadCreate(packet.command, packet.value, client_fd);
                        break;
                    case CMD_SEGMENT_STOP:
                        printf("7세그먼트 카운트다운 중단 명령\n");
                        myThreadCreate(packet.command, packet.value, client_fd);
                        break;
                    case CMD_SEGMENT_INC:
                        printf("7세그먼트 수동 카운트 증가 명령\n");
                        myThreadCreate(packet.command, packet.value, client_fd);
                        break;
                    default:
                        printf("알 수 없는 알 수 없는 명령어입니다.\n");
                        break;
                }

                // 클라이언트가 0번(종료)을 보냈다면 수신 루프 탈출
                if (packet.command == CMD_EXIT) {
                    break;
                }

            } else if (recv_bytes == 0) {
                // 클라이언트가 정상적으로 소켓을 닫고 나간 경우 (예: Ctrl+C로 강제 종료 등)
                printf("[연결 끊김] 클라이언트가 연결을 해제했습니다.\n");
                break;
            } else {
                // 수신 에러 발생 시
                perror("데이터 수신 오류");
                break;
            }
        }

        // 사용이 끝난 클라이언트 소켓을 닫고 다음 클라이언트를 맞이할 준비를 합니다.
        close(client_fd);
        printf("[알림] 클라이언트 세션 종료. 리슨 상태로 복귀합니다.\n");
    }

    // 이 코드는 메인 루프가 무한대라 실제 도달하진 않지만 안전을 위해 작성합니다.
    if (server_fd >= 0) close(server_fd);
    return 0;
}