# ESP32 E-Paper Display Project with LVGL

## 개요
이 프로젝트는 ESP32와 2.13인치 E-Paper 디스플레이를 사용하여 LVGL 그래픽 라이브러리로 UI를 구현한 프로젝트입니다.

## 하드웨어 요구사항
- MCU: ESP32
- 디스플레이: 2.13인치 E-Paper Display (104 x 212)
- GPIO 연결:
  - BUSY: GPIO 4
  - RST: GPIO 16
  - DC: GPIO 17
  - CS: GPIO 5
  - CLK: GPIO 18
  - MOSI: GPIO 23

## 소프트웨어 요구사항
- ESP-IDF 버전: v4.1.0 이상
- LVGL 버전: v8.4.0
- 컴포넌트:
  - lvgl/lvgl: ^8.3.11

## 프로젝트 구조
```bash
├── CMakeLists.txt
├── main
│ ├── CMakeLists.txt
│ ├── main.c
│ └── idf_component.yml
├── components
│ └── epaper
│ ├── CMakeLists.txt
│ ├── epaper.c
│ └── include
│ └── epaper.h
└── README.md
```

## 빌드 및 실행 방법
1. ESP-IDF 환경 설정
```bash
. $HOME/esp/esp-idf/export.sh
```

2. 프로젝트 빌드
```bash
idf.py build
```

3. 플래시 및 모니터링
```bash
idf.py -p (PORT) flash monitor
```

## 주요 기능
- LVGL 기반 UI 구현
- E-Paper 디스플레이 드라이버 구현
- 버튼 인터럽트 처리
- 저전력 모드 지원 (Deep Sleep)

## 참고 문서
- [LVGL 문서](https://docs.lvgl.io/)
- [ESP-IDF 프로그래밍 가이드](https://docs.espressif.com/projects/esp-idf/)

## 라이선스
이 프로젝트는 MIT 라이선스를 따릅니다.