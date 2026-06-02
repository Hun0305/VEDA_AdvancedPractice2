# 컴파일러 및 옵션 설정
CC = gcc
CFLAGS = -Wall -Wextra -O2

# 디렉토리 경로 정의
CODE_DIR = code
EXEC_DIR = exec

# 최종 생성될 실행 파일 경로
CLIENT_EXE = $(EXEC_DIR)/client/client_app
SERVER_EXE = $(EXEC_DIR)/server/server_main

# 기본 타겟 (make 또는 make all 입력 시 실행)
all: directories $(CLIENT_EXE) $(SERVER_EXE)

# 1. 필요한 exec 하위 디렉토리 자동 생성
directories:
	@mkdir -p $(EXEC_DIR)/client $(EXEC_DIR)/server

# 2. 클라이언트 빌드 (client_main.c 단일 컴파일)
$(CLIENT_EXE): $(CODE_DIR)/client/client_main.c
	$(CC) $(CFLAGS) $< -o $@

# 3. 서버 빌드 (server_main.c 단일 컴파일)
$(SERVER_EXE): $(CODE_DIR)/server/server_main.c
	$(CC) $(CFLAGS) $< -o $@

# 빌드된 실행 파일 전체 삭제 (make clean)
clean:
	rm -rf $(EXEC_DIR)/client/* $(EXEC_DIR)/server/*

.PHONY: all directories clean