// code/server/tcp_handler.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dlfcn.h>
#include "../include/protocol.h"
#include "../include/device_api.h"

// server_main에서 이미 파싱된 command와 value를 그대로 받음
void handle_device_command(int command, int value) {
    char *lib_path = NULL;

    // 1. 명령어 종류에 따라 열어야 할 공유 라이브러리(.so) 경로 지정
    if (command == CMD_LED_ON || command == CMD_LED_OFF || command == CMD_SET_BRIGHT) {
        lib_path = "exec/lib/libled.so";
    }
    
    //추후 다른 장치 구현 시 추가 영역
    else if (command == CMD_BUZZER_ON || command == CMD_BUZZER_OFF) {
        lib_path = "exec/lib/libbuzzer.so";
    }
    

    if (lib_path == NULL) return; // 제어 대상 장치가 아니거나 EXIT 등인 경우 예외처리

    // 2. 라이브러리 로드
    void *handle = dlopen(lib_path, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "[서버 에러] 라이브러리 로드 실패: %s\n", dlerror());
        return;
    }

    // 3. 함수 주소 획득
    DeviceInitFunc    dev_init    = (DeviceInitFunc)dlsym(handle, "device_init");
    DeviceControlFunc dev_control = (DeviceControlFunc)dlsym(handle, "device_control");
    DeviceReleaseFunc dev_release = (DeviceReleaseFunc)dlsym(handle, "device_release");

    if (dlerror() != NULL) {
        fprintf(stderr, "[서버 에러] 함수 심볼 로드 실패\n");
        dlclose(handle);
        return;
    }

    // 4. 하드웨어 제어 시퀀스 실행
    if (dev_init() == 0) {
        dev_control(command, value); // .so 내부의 제어 로직 호출
        dev_release();
    }

    // 5. 라이브러리 해제
    // dlclose(handle);
}

/* ★ 스레드가 실제로 진입해서 실행할 함수 ★ */
void* device_command_thread(void* arg) {
    // 1. 넘겨받은 포인터를 구조체 타입으로 캐스팅
    ThreadArgs* args = (ThreadArgs*)arg;

    int cmd = args->command;
    int val = args->value;

    free(args);

    // 2. 실제 장치 제어 함수 호출
    handle_device_command(cmd, val);

    return NULL;
}