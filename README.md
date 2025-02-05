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

## 소프트웨어 테스트 스펙
- ESP-IDF 버전: v5.4.0
- LVGL 버전: v8.3.11
- LVGL 버전: v9.1.0

## 주요 기능
- LVGL 기반 UI 구현
- E-Paper 디스플레이 드라이버 구현
- 버튼 인터럽트 처리
- 화면 회전 기능 추가

## 참고 문서
- [LVGL 문서](https://docs.lvgl.io/)
- [ESP-IDF 프로그래밍 가이드](https://docs.espressif.com/projects/esp-idf/)

## 라이선스
이 프로젝트는 MIT 라이선스를 따릅니다.