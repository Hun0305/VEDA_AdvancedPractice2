#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>
#include <softTone.h> // Software PWM 음정 제어를 위해 필수
#include "../include/device_api.h"
#include "../include/protocol.h"

// BCM GPIO 21번은 WiringPi 번호로 29번입니다.
#define BUZZER_PIN 29

// 음정 주파수 정의
#define NOTE_C  262 // 도
#define NOTE_D  294 // 레
#define NOTE_E  330 // 미
#define NOTE_F  349 // 파
#define NOTE_G  392 // 솔
#define NOTE_A  440 // 라
#define NOTE_B  494 // 시
#define NOTE_C5 523 // 높은 도

/*
 * 1. 장치 초기화 (softTone 기능 활성화)
 */
int device_init(void) {
    if (wiringPiSetup() == -1) {
        fprintf(stderr, "[부저 라이브러리] wiringPi 초기화 실패\n");
        return -1;
    }

    // softToneCreate(핀 번호) 호출하여 소프트웨어 톤 제어 초기화
    if (softToneCreate(BUZZER_PIN) != 0) {
        fprintf(stderr, "[부저 라이브러리] softTone 초기화 실패\n");
        return -1;
    }

    printf("[부저 라이브러리] GPIO 및 softTone 초기화 완료 (Active-High 구조)\n");
    return 0;
}

/*
 * 2. 장치 제어 (멜로디 재생 및 중지)
 */
int device_control(int command, int value) {
    (void)value; // 부저 메뉴에서는 value를 사용하지 않으므로 경고 방지

    switch (command) {
        case CMD_BUZZER_ON:
            printf("[부저 라이브러리] 멜로디 재생 시작 (도-미-솔-도)\n");
            
            // 간단한 도-미-솔-도 멜로디 연주
            int melody[] = {NOTE_C, NOTE_E, NOTE_G, NOTE_C5};
            int duration = 200000; // 각 음정당 연주 시간 (200ms = 200,000us)

            for (int i = 0; i < 4; i++) {
                softToneWrite(BUZZER_PIN, melody[i]); // 해당 주파수 출력
                usleep(duration);                    // 유지
            }
            
            // 연주가 끝나면 소리를 끕니다.
            softToneWrite(BUZZER_PIN, 0); 
            break;

        case CMD_BUZZER_OFF:
            printf("[부저 라이브러리] 부저 강제 정지\n");
            // 주파수를 0으로 주면 출력이 차단되어 소리가 멈춥니다.
            softToneWrite(BUZZER_PIN, 0);
            break;

        default:
            fprintf(stderr, "[부저 라이브러리] 지원하지 않는 명령어: %d\n", command);
            return -1;
    }
    return 0;
}

/*
 * 3. 자원 해제 (안전하게 끄기)
 */
int device_release(void) {
    // 종료 시 부저가 울리고 있다면 확실하게 소등
    softToneWrite(BUZZER_PIN, 0);
    printf("[부저 라이브러리] 자원 해제 완료\n");
    return 0;
}