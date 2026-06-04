// code/lib_src/fnd.c

#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>
#include "../include/device_api.h"
#include "../include/protocol.h"

// 74LS47 BCD 입력 핀 설정 (반드시 본인의 회로 연결에 맞게 WiringPi 번호로 수정하세요!)
// A가 LSB(가장 낮은 자리), D가 MSB(가장 높은 자리) 입니다.
#define FND_A 15 
#define FND_B 16
#define FND_C 1
#define FND_D 4 

// 카운트다운을 중간에 멈추기 위한 전역 플래그
volatile int fnd_running = 0;

// 숫자를 이진수(BCD)로 변환하여 4개의 핀에 출력하는 헬퍼 함수
void display_number(int num) {
    digitalWrite(FND_A, (num & 0x01) ? HIGH : LOW);
    digitalWrite(FND_B, (num & 0x02) ? HIGH : LOW);
    digitalWrite(FND_C, (num & 0x04) ? HIGH : LOW);
    digitalWrite(FND_D, (num & 0x08) ? HIGH : LOW);
}

int device_init(void) {
    if (wiringPiSetup() == -1) return -1;
    
    pinMode(FND_A, OUTPUT);
    pinMode(FND_B, OUTPUT);
    pinMode(FND_C, OUTPUT);
    pinMode(FND_D, OUTPUT);
    
    // 74LS47 칩은 15(1111)를 입력하면 모든 불빛이 꺼집니다(Blanking). 초기화 시 화면 소등.
    display_number(15); 
    return 0;
}

int device_control(int command, int value) {
    if (command == CMD_SEGMENT_DISP) {
        if (value > 9) value = 9; // 9 초과 입력 시 9로 고정
        
        fnd_running = 1;
        printf("[FND] 카운트다운 시작: %d\n", value);
        
        // value부터 0까지 카운트다운
        for (int i = value; i >= 0 && fnd_running; i--) {
            display_number(i);
            
            if (i == 0) {
                // 0에 도달하면 부저를 울려야 하므로, 리턴값 1을 상위 스레드로 던져줍니다.
                fnd_running = 0;
                return 1; 
            }
            
            // 1초를 대기하되, STOP 명령이 오면 즉각 반응하도록 0.1초씩 10번 쪼개서 검사합니다.
            for (int j = 0; j < 10 && fnd_running; j++) {
                usleep(100000); 
            }
        }
    } 
    else if (command == CMD_SEGMENT_STOP) {
        printf("[FND] 카운트다운 중단 요청 수신!\n");
        fnd_running = 0;    // 진행 중인 카운트다운 루프 정지
        display_number(15); // 화면 불빛 완전히 끄기
    }
    
    return 0; // 정상 종료(도중에 멈춤) 시 0 리턴
}

int device_release(void) {
    fnd_running = 0;
    display_number(15);
    return 0;
}