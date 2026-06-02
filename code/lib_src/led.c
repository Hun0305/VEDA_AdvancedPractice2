#include <stdio.h>
#include <wiringPi.h>
#include "../include/device_api.h"
#include "../include/protocol.h"

// LED가 연결된 WiringPi 핀 번호 정의 
// (BCM GPIO 12번은 WiringPi 번호로 26번입니다. 만약 BCM 번호를 쓰고 싶다면 wiringPiSetupGpio()를 사용해야 합니다.)
#define LED_PIN 26

// Active-Low 구조에 따른 매크로 정의
#define LED_ON_VAL  LOW
#define LED_OFF_VAL HIGH

/*
 * 1. 장치 초기화 (wiringPi 설정 및 핀 모드 지정)
 */
int device_init(void) {
    // wiringPi 초기화 (이미 서버 메인에서 수행했다면 중복 호출해도 안전합니다)
    if (wiringPiSetup() == -1) {
        fprintf(stderr, "[LED 라이브러리] wiringPi 초기화 실패\n");
        return -1;
    }

    // 초기 상태는 LED가 꺼진 상태(HIGH)로 설정 후 출력 모드로 변경
    digitalWrite(LED_PIN, LED_OFF_VAL);
    pinMode(LED_PIN, OUTPUT);
    
    printf("[LED 라이브러리] GPIO 초기화 완료 (Active-Low 구조)\n");
    return 0;
}

/*
 * 2. 장치 제어 (ON, OFF 및 PWM 밝기 조절)
 */
int device_control(int command, int value) {
    switch (command) {
        case CMD_LED_ON:
            // PWM 모드였다가 일반 ON으로 올 수 있으므로 다시 OUTPUT 모드로 전환
            pinMode(LED_PIN, OUTPUT);
            digitalWrite(LED_PIN, LED_ON_VAL);
            printf("[LED 라이브러리] LED 켜짐 (LOW 출력)\n");
            break;

        case CMD_LED_OFF:
            pinMode(LED_PIN, OUTPUT);
            digitalWrite(LED_PIN, LED_OFF_VAL);
            printf("[LED 라이브러리] LED 꺼짐 (HIGH 출력)\n");
            break;

        case CMD_SET_BRIGHT:
            // PWM 제어를 위해 핀 모드를 PWM_OUTPUT으로 변경
            pinMode(LED_PIN, PWM_OUTPUT);
            
            // wiringPi의 하드웨어 PWM 범위 기본값은 0 ~ 1023입니다.
            // Active-Low 구조이므로 듀티 사이클(Duty Cycle)을 반대로 계산해야 합니다.
            // 0에 가까울수록 LOW 시간이 길어져서 밝아지고, 1023에 가까울수록 꺼집니다.
            int pwm_value = 1023; 

            switch (value) {
                case BRIGHT_LOW:
                    // 살짝만 켜지게 함 -> HIGH 시간을 길게, LOW 시간을 짧게 (예: 800)
                    pwm_value = 850;
                    printf("[LED 라이브러리] 밝기 조절: 낮음 (PWM Value: %d)\n", pwm_value);
                    break;
                case BRIGHT_MID:
                    // 중간 밝기 (예: 512)
                    pwm_value = 512;
                    printf("[LED 라이브러리] 밝기 조절: 중간 (PWM Value: %d)\n", pwm_value);
                    break;
                case BRIGHT_MAX:
                    // 최대 밝기 -> 완전히 LOW로 유지 (0)
                    pwm_value = 0;
                    printf("[LED 라이브러리] 밝기 조절: 최대 (PWM Value: %d)\n", pwm_value);
                    break;
                default:
                    fprintf(stderr, "[LED 라이브러리] 잘못된 밝기 단계: %d\n", value);
                    return -1;
            }
            
            // 하드웨어 PWM 신호 출력
            pwmWrite(LED_PIN, pwm_value);
            break;

        default:
            fprintf(stderr, "[LED 라이브러리] 지원하지 않는 명령어 코드: %d\n", command);
            return -1;
    }
    return 0;
}

/*
 * 3. 자원 해제 (종료 시 LED를 안전하게 끄기)
 */
int device_release(void) {
    // 종료 시에는 일반 출력 모드로 돌린 후 핀을 HIGH로 만들어 LED를 완전히 끕니다.
    // pinMode(LED_PIN, OUTPUT);
    // digitalWrite(LED_PIN, LED_OFF_VAL);
    // printf("[LED 라이브러리] 자원 해제 및 LED 완전 소등\n");
    return 0;
}