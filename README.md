# 🖥️ TCP를 이용한 원격 장치 제어 프로그램

> **VEDA 한화비전 교육과정** — 심화 실습 평가 2 (리눅스 프로그래밍)

---

## 📌 프로젝트 개요

라즈베리파이 4(서버)와 Ubuntu Linux(클라이언트) 간 **TCP 소켓 통신**을 이용하여  
LED, 부저, 조도센서, 7세그먼트 디스플레이를 원격으로 제어하는 시스템입니다.

| 구분 | 사양 |
|------|------|
| Server | Raspberry Pi 4 |
| Client | Ubuntu Linux |
| Protocol | TCP/IP |
| Language | C (gcc) |
| Build | Makefile |

---

## ✅ 구현 기능 체크리스트

### 장치 제어 (40점)

| 장치 | 기능 | 구현 |
|------|------|:----:|
| **LED** | 클라이언트에서 ON/OFF 및 밝기(최대/중간/최저) 조절 | ✅ |
| **부저** | 클라이언트에서 (음악) 소리 ON/OFF 제어 | ✅ |
| **조도센서** | 조도 값 확인, 빛 감지 여부에 따른 LED 자동 제어 | ✅ |
| **7세그먼트** | 숫자(0~9) 표시, 1초 카운트다운, 0 도달 시 부저 울림 | ✅ |

### 구현 내용 (30점)

| 항목 | 구현 |
|------|:----:|
| 멀티 프로세스 또는 스레드 이용한 장치 제어 | ✅ |
| 장치 제어 프로그램을 **공유 라이브러리(.so)** 형식으로 작성 | ✅ |
| 장치 기능 업그레이드 시 `.so` 파일만 교체하여 수행 가능 (동적 라이브러리) | ✅ |
| 클라이언트 프로그램 강제 종료 방지 시그널 처리 (INT 신호) | ✅ |
| 서버 **Daemon 프로세스** 형식으로 구성 | ✅ |
| 빌드 자동화 (**Makefile**) | ✅ |

---

## 📁 디렉토리 구조

```
VEDA_AdvancedPractice2/
├── code/
│   ├── client/
│   │   └── client_main.c       # 클라이언트 소스 (메뉴 UI, TCP, 시그널 처리)
│   ├── server/
│   │   ├── server_main.c       # 서버 메인 (데몬 프로세스, TCP 대기)
│   │   └── tcp_handler.c       # TCP 연결 처리 모듈
│   └── lib_src/
│       ├── led.c               # LED 제어 라이브러리
│       ├── buzzer.c            # 부저 제어 라이브러리
│       ├── cds.c               # 조도센서 라이브러리
│       └── fnd.c               # 7세그먼트 라이브러리
├── exec/
│   ├── client/client_app       # 클라이언트 실행 파일
│   ├── server/server_main      # 서버 실행 파일
│   └── lib/
│       ├── libled.so
│       ├── libbuzzer.so
│       ├── libcnd.so
│       └── libfnd.so
├── docs/
│   ├── manual.pdf              # 개발 문서
│   └── README.md
├── misc/                       # 회로도 등 기타 파일
├── Makefile
└── .gitignore
```

---

## ⚙️ 빌드 방법

### 사전 요구사항

**서버 (Raspberry Pi 4)**
```bash
sudo apt-get install wiringpi
```

**클라이언트 (Ubuntu)**
```bash
sudo apt-get install build-essential
```

### 빌드

```bash
# 전체 빌드 (클라이언트 + 서버 실행 파일 + 모든 .so 라이브러리)
make

# 공유 라이브러리(.so)만 빌드
make libs

# 빌드 결과물 삭제
make clean
```

---

## 🚀 실행 방법

**서버 실행 (Raspberry Pi)**
```bash
./exec/server/server_main
```

**클라이언트 실행 (Ubuntu)**
```bash
./exec/client/client_app
```

실행 시 아래와 같은 메뉴가 표시됩니다:

```
[ Device Control Menu ]
1. LED ON
2. LED OFF
3. Set Brightness
4. BUZZER ON (play melody)
5. BUZZER OFF (stop)
6. SENSOR ON  (감지 시작)
7. SENSOR OFF (감지 종료)
8. SEGMENT DISPLAY (숫자 표시 후 카운트다운)
9. SEGMENT STOP    (카운트다운 중단)
+ 10. SEGMENT INC (수동 카운터 증가)
0. Exit
Select:
```

---

## 🏗️ 아키텍처

```
[ Ubuntu Client ]                    [ Raspberry Pi 4 Server ]
+------------------+    TCP/IP    +---------------------------+
|  client_main.c   | ----------> |  server_main.c (Daemon)   |
|  - 메뉴 UI        |             |  tcp_handler.c            |
|  - SIGINT 처리   |             |  dlopen() → .so 로드      |
+------------------+             +---------------------------+
                                          |
                    ┌─────────────────────┼─────────────────────┐
                    ▼                     ▼                     ▼                     ▼
              libled.so           libbuzzer.so            libcnd.so             libfnd.so
              (PWM, BCM12)        (wiringPi)              (wiringPi)            (74LS47)
```

### 주요 구현 사항

- **동적 라이브러리(dlopen)**: 각 장치 제어 기능을 `.so`로 분리, `dlopen/dlsym`으로 런타임 로드 → `.so` 파일만 교체하여 기능 업그레이드 가능
- **Daemon 프로세스**: `fork() + setsid()`로 서버를 백그라운드 데몬으로 구동
- **멀티스레드(pthread)**: 장치 제어 및 7세그먼트 카운트다운을 스레드로 처리
- **시그널 처리**: `SIGINT`만 핸들링하여 소켓 정리 후 안전 종료

---

## 🔌 하드웨어 연결

| 장치 | 핀/칩 | 비고 |
|------|-------|------|
| LED | BCM12 (PWM0_0) | PWM 밝기 제어 |
| 부저 | GPIO | 멜로디 재생 |
| 조도센서 (CDS) | GPIO | 아날로그 → 디지털 |
| 7세그먼트 | 74LS47 디코더 | BCD 입력 |
