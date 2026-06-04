#include <stdio.h>
#include <wiringPi.h>
#include "../include/device_api.h"
#include "../include/protocol.h"

// BCM 11 = WiringPi 14
#define SENSOR_PIN 14
// BCM 12 = WiringPi 26 (기존 LED 핀)
#define LED_PIN    26 

int device_init(void) {
    if (wiringPiSetup() == -1) {
        return -1;
    }
    pinMode(SENSOR_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT); // 센서 연동 제어를 위해 LED 핀도 세팅
    return 0;
}

/*
 * 장치 제어 및 값 반환
 * value 매개변수 주소를 통해 센서 값을 역으로 상위 레이어에 전달할 수 있도록 구성합니다.
 */
// code/lib_src/cds.c 내부 수정

int device_control(int command, int value) {
    (void)value; // 이제 value 매개변수는 포인터로 쓰지 않으므로 경고 방지 처리

    if (command == CMD_SENSOR_ON) {
        int state = digitalRead(SENSOR_PIN);
        
        // 디버그용 출력
        printf("[CDS] 센서 상태: %d\n", state);

        if (state == HIGH) {
            digitalWrite(LED_PIN, LOW);   // Active-Low 구조: LED ON
            return 1;                     // 밝음(빛 감지) 상태를 숫자로 직접 리턴
        } else {
            digitalWrite(LED_PIN, HIGH);  // LED OFF
            return 0;                     // 어두움 상태를 숫자로 직접 리턴
        }
    } 
    else if (command == CMD_SENSOR_OFF) {
        digitalWrite(LED_PIN, HIGH);
        return 0;
    }
    
    return 0;
}

int device_release(void) {
    return 0;
}