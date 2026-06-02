#ifndef DEVICE_API_H
#define DEVICE_API_H

// 모든 장치 라이브러리가 공통으로 구현해야 하는 함수 인터페이스
int device_init(void);
int device_control(int command, int value);
int device_release(void);


/* 명시적 동적 링크(dlopen, dlsym)를 위한 함수 포인터 타입 정의
 * server_main.c 또는 tcp_handler.c에서 dlsym()의 반환값을 캐스팅할 때 사용합니다.
 */

// 장치 초기화 함수 포인터 타입 (매개변수 없음, int 반환)
typedef int (*DeviceInitFunc)(void);

// 장치 제어 함수 포인터 타입 (command와 value를 받아 하드웨어 제어)
typedef int (*DeviceControlFunc)(int, int);

// 장치 해제 및 자원 반환 함수 포인터 타입 (매개변수 없음, int 반환)
typedef int (*DeviceReleaseFunc)(void);

#endif // DEVICE_API_H