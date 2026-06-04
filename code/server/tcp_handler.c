// code/server/tcp_handler.c

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dlfcn.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include "../include/protocol.h"
#include "../include/device_api.h"

// ★ 1. 스레드 루프를 제어할 전역 플래그 선언 (volatile을 붙여 최적화 방지)
volatile int sensor_running = 0;

void handle_device_command(int command, int value) {
    char *lib_path = NULL;

    if (command == CMD_LED_ON || command == CMD_LED_OFF || command == CMD_SET_BRIGHT) {
        lib_path = "exec/lib/libled.so";
    }
    else if (command == CMD_BUZZER_ON || command == CMD_BUZZER_OFF) {
        lib_path = "exec/lib/libbuzzer.so";
    }

    // ★ 9번 STOP 명령이 왔을 때 FND 라이브러리를 열어서 중단 신호를 보내도록 추가
    else if (command == CMD_SEGMENT_STOP || command == CMD_SEGMENT_INC) {
        lib_path = "exec/lib/libfnd.so"; 
    }

    if (lib_path == NULL) return; 

    void *handle = dlopen(lib_path, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "[서버 에러] 라이브러리 로드 실패: %s\n", dlerror());
        return;
    }

    DeviceInitFunc    dev_init    = (DeviceInitFunc)dlsym(handle, "device_init");
    DeviceControlFunc dev_control = (DeviceControlFunc)dlsym(handle, "device_control");
    DeviceReleaseFunc dev_release = (DeviceReleaseFunc)dlsym(handle, "device_release");

    if (dlerror() != NULL) {
        // dlclose(handle);
        return;
    }

    if (dev_init() == 0) {
        dev_control(command, value); 
        dev_release();
    }
    
    // 일회성 장치들은 사용 후 즉시 닫아줍니다.
    // dlclose(handle);
}

/* ★ 스레드가 실제로 진입해서 실행할 함수 ★ */
void* device_command_thread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;

    int cmd = args->command;
    int val = args->value;
    int client_fd = args->client_fd; 

    free(args);

    if (cmd == CMD_SENSOR_ON) {
        // 이미 실행 중이라면 스레드 중복 생성을 막습니다.
        if (sensor_running) return NULL; 

        printf("[스레드] CMD_SENSOR_ON 명령 처리 시작 (클라이언트 FD: %d)\n", client_fd);
        void *handle = dlopen("exec/lib/libcnd.so", RTLD_LAZY);
        if (!handle) return NULL; 

        DeviceInitFunc    dev_init    = (DeviceInitFunc)dlsym(handle, "device_init");
        DeviceControlFunc dev_control = (DeviceControlFunc)dlsym(handle, "device_control");
        
        if (dev_init() == 0) {
            // ★ 2. 플래그를 1로 켜고, while(1) 대신 while(sensor_running)으로 변경
            sensor_running = 1;
            
            while (sensor_running) {
                // ★ 3. (중요) 세그폴트 방지를 위해 주소가 아닌 리턴값으로 받기!
                int sensor_val = dev_control(CMD_SENSOR_ON, 0);

                SensorResponsePacket res;
                res.type = RESP_SENSOR_DATA;
                res.light_detected = sensor_val;

                if (send(client_fd, &res, sizeof(res), 0) <= 0) {
                    printf("[스레드] 클라이언트 연결 끊김으로 센서 루프 종료\n");
                    break;
                }
                
                usleep(500000); 
            }
            
            // 루프를 탈출했다면(OFF 수신), 센서와 LED를 확실하게 꺼줍니다.
            dev_control(CMD_SENSOR_OFF, 0);
        }
        // dlclose(handle);
        sensor_running = 0; // 플래그 초기화

    } 
    // ★ 4. OFF 명령이 들어왔을 때의 분기 추가
    else if (cmd == CMD_SENSOR_OFF) {
        printf("[스레드] 조도센서 감시 중지 요청 수신! 루프를 종료합니다.\n");
        // 이 값을 0으로 바꾸면 백그라운드에서 돌고 있던 위쪽 while문이 거짓이 되어 탈출합니다.
        sensor_running = 0; 
    } 
    // ★ 8번: FND 카운트다운 스레드 분기 추가
    else if (cmd == CMD_SEGMENT_DISP) {
        printf("[스레드] CMD_SEGMENT_DISP 명령 처리 시작 (시작 숫자: %d)\n", val);
        
        void *handle = dlopen("exec/lib/libfnd.so", RTLD_LAZY);
        if (!handle) return NULL;
        
        DeviceInitFunc    dev_init    = (DeviceInitFunc)dlsym(handle, "device_init");
        DeviceControlFunc dev_control = (DeviceControlFunc)dlsym(handle, "device_control");
        
        if (dev_init() == 0) {
            // fnd.c의 카운트다운이 끝날 때까지 여기서 스레드가 대기합니다.
            int result = dev_control(CMD_SEGMENT_DISP, val);
            
            // 카운트다운이 도중에 취소되지 않고 무사히 0에 도달하여 1을 리턴했다면?
            if (result == 1) {
                printf("[스레드] 카운트다운 완료! 부저를 1초간 울립니다.\n");
                
                // 기존에 만들어둔 단발성 제어 함수를 불러 부저를 켭니다!
                handle_device_command(CMD_BUZZER_ON, 0);
                usleep(1000000); // 1초 유지
                handle_device_command(CMD_BUZZER_OFF, 0);
            }
        }
    } 
    else {
        // 기존 LED, 부저 처리 로직
        handle_device_command(cmd, val);
    }

    return NULL;
}