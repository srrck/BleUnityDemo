# BleUnityDemo

[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## 소개

**BleUnityDemo**는 **Windows 환경 전용** Unity 프로젝트로, Bluetooth Low Energy (BLE) 장치를 검색하고 연결하며 데이터를 주고받는 기능을 제공합니다.

이 프로젝트는 주변의 BLE 장치를 검색하고, 연결하고, 데이터를 주고받는 기본적인 흐름을 이해하고 Unity 애플리케이션에 통합하는 데 도움을 줍니다.

> 이 프로젝트는 Windows BLE 기능 구현을 위해 [adabru/BleWinrtDll](https://github.com/adabru/BleWinrtDll) 프로젝트를 참고했습니다.

> 또한 본 프로젝트의 일부 코드 및 구조는 ChatGPT, Gemini, GitHub Copilot 등 AI 도구의 제안을 기반으로 작성되었습니다.

## 🎯 주요 기능

- Windows에서 BLE 장치 검색 및 목록 표시
- BLE 장치 연결 및 서비스/특성 탐색
- 특성 데이터 읽기 / 쓰기 / 알림 수신
- Unity UI를 통한 인터랙티브한 BLE 통신

## 💻 요구 사항

- **운영체제**: Windows 10 이상
- **Unity 버전**: 2021.x 이상 권장
- **DLL 의존성**: BLEWin.dll (WinRT 기반 C++ DLL)
- **BLE 동글**: BLE 4.0 이상을 지원하는 어댑터 필요
- **UnityMainThreadDispatcher**: BLE 알림을 Unity 메인 스레드에서 안전하게 처리하기 위한 필수 유틸리티  
  설치 방법 👉 [https://github.com/PimDeWitte/UnityMainThreadDispatcher](https://github.com/PimDeWitte/UnityMainThreadDispatcher)

## ⚙️ 설치 방법

1. 이 저장소를 클론하거나 `.unitypackage` 또는 `.zip`을 다운로드합니다.
2. Unity에서 프로젝트를 열고, `BLEWin.dll`을 `Assets/Plugins/x86_64/` 폴더에 넣습니다.
3. [UnityMainThreadDispatcher](https://github.com/PimDeWitte/UnityMainThreadDispatcher)를 설치합니다.
4. `SampleScene.unity` 씬을 실행합니다.
5. Scan → 장치 선택 → 서비스 조회 → 특성 Subscribe 순으로 동작합니다.

## 📝 라이선스

이 프로젝트는 MIT 라이선스를 따릅니다. 자세한 내용은 LICENSE 파일을 참조하세요.
