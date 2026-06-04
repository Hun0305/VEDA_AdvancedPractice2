# 컴파일러 및 옵션 설정
CC = gcc
CFLAGS = -Wall -Wextra -O2

# 디렉토리 경로 정의
CODE_DIR = code
EXEC_DIR = exec
LIB_DIR = code/lib_src

# 최종 생성될 실행 파일 및 라이브러리 경로
CLIENT_EXE = $(EXEC_DIR)/client/client_app
SERVER_EXE = $(EXEC_DIR)/server/server_main
LED_SO     = $(EXEC_DIR)/lib/libled.so
BUZZER_SO = $(EXEC_DIR)/lib/libbuzzer.so
CDS_SO = $(EXEC_DIR)/lib/libcnd.so
FND_SO = $(EXEC_DIR)/lib/libfnd.so

# 기본 타겟 (make 또는 make all 입력 시 실행)
# 이제 실행 파일뿐만 아니라 libs(즉, .so 파일)도 함께 빌드 타겟에 묶어줍니다.
all: directories $(CLIENT_EXE) $(SERVER_EXE) libs

# 1. 필요한 exec 하위 디렉토리 자동 생성 (lib 폴더도 추가)
directories:
	@mkdir -p $(EXEC_DIR)/client $(EXEC_DIR)/server $(EXEC_DIR)/lib $(LIB_DIR)

# 2. 클라이언트 빌드 (client_main.c 단일 컴파일)
$(CLIENT_EXE): $(CODE_DIR)/client/client_main.c
	$(CC) $(CFLAGS) $< -o $@

# 3. 서버 빌드 (server_main.c와 tcp_handler.c 링크 및 라이브러리 옵션 추가)
$(SERVER_EXE): $(CODE_DIR)/server/server_main.c $(CODE_DIR)/server/tcp_handler.c
	$(CC) $(CFLAGS) $^ -o $@ -ldl -lpthread

# 4. 공유 라이브러리(.so) 빌드 타겟 추가
libs: $(LED_SO) $(BUZZER_SO) $(CDS_SO) $(FND_SO)

# libled.so 빌드 규칙
# -fPIC (위치 독립 코드)와 -shared (공유 라이브러리) 옵션이 필수입니다.
# 라즈베리 파이 하드웨어 제어를 위해 -lwiringPi 링크 옵션을 붙여줍니다.
$(LED_SO): $(LIB_DIR)/led.c
	$(CC) $(CFLAGS) -fPIC -shared $< -o $@ -lwiringPi

$(BUZZER_SO): $(LIB_DIR)/buzzer.c
	$(CC) $(CFLAGS) -fPIC -shared $< -o $@ -lwiringPi

$(CDS_SO): $(LIB_DIR)/cds.c
	$(CC) $(CFLAGS) -fPIC -shared $< -o $@ -lwiringPi

$(FND_SO): $(LIB_DIR)/fnd.c
	$(CC) $(CFLAGS) -fPIC -shared $< -o $@ -lwiringPi

# 빌드된 파일 전체 삭제 (make clean)
clean:
	rm -rf $(EXEC_DIR)/client/* $(EXEC_DIR)/server/* $(EXEC_DIR)/lib/*

.PHONY: all directories libs clean