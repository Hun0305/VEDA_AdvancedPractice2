#ifndef PROTOCOL_H
#define PROTOCOL_H

// TCP 통신에 사용할 포트 번호
#define DEFAULT_PORT 8080
#define SERVER_IP "172.20.35.70"

// 명령어(Command) 정의 (클라이언트 메뉴와 매핑)
#define CMD_EXIT          0
#define CMD_LED_ON        1
#define CMD_LED_OFF       2
#define CMD_SET_BRIGHT    3
#define CMD_BUZZER_ON     4
#define CMD_BUZZER_OFF    5
#define CMD_SENSOR_ON     6
#define CMD_SENSOR_OFF    7
#define CMD_SEGMENT_DISP  8
#define CMD_SEGMENT_STOP  9

// LED 밝기 상세 값 (CMD_SET_BRIGHT 선택 시 value에 담길 값)
#define BRIGHT_LOW        1
#define BRIGHT_MID        2
#define BRIGHT_MAX        3

/* * 클라이언트와 서버가 주고받을 기본 데이터 패킷 구조체 
 * fixed-size 구조체로 설계하여 소켓을 통해 통째로 send/recv 하기 용이합니다.
 */
typedef struct {
    int command;    // 명령어 종류 (CMD_EXI ~ CMD_SEGMENT_STOP)
    int value;      // 추가 데이터 (밝기 단계 또는 세그먼트 카운트다운 시작 숫자)
} DevicePacket;

/*
 * [선택 사항] 서버가 명령을 처리한 후 클라이언트에게 응답(ACK)을 보낼 때 사용할 구조체
 */
typedef struct {
    int status;     // 0: 성공(SUCCESS), -1: 실패(FAIL)
    char message[64]; // 처리 결과 메시지
} ResponsePacket;

#endif // PROTOCOL_H