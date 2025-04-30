BleUnityDemo (Windows 전용)

소개
BleUnityDemo는 Unity 환경에서 Windows 운영체제의 Bluetooth Low Energy (BLE) 기능을 활용하는 방법을 보여주는 데모 프로젝트입니다. 이 프로젝트는 주변의 BLE 장치를 검색하고, 연결하고, 데이터를 주고받는 기본적인 흐름을 이해하고 Unity 애플리케이션에 통합하는 데 도움을 드립니다.

이 프로젝트의 내용은 ChatGPT와 Gemini를 활용하여 작성되었으며, Windows 환경에서의 BLE 기능 구현을 위해 https://github.com/adabru/BleWinrtDll 프로젝트를 참고했습니다. 해당 프로젝트의 기여자분들께 감사드립니다.

주요 목표:

Unity에서 BLE 플러그인 (BleWinrtDll 활용)을 사용하여 Windows 환경에서 BLE 기능을 구현하는 기본적인 예제 제공
주변 BLE 장치 검색 및 목록 표시
특정 BLE 장치에 연결 및 연결 해제
BLE 서비스 및 특성(Characteristic) 검색
특정 특성의 값 읽기 및 쓰기
특성 값 변경에 대한 알림(Notification) 수신
데모 (선택 사항)
설치 방법
이 프로젝트를 사용하기 위해서는 다음과 같은 환경이 필요합니다.

Unity: 20xx.x 버전 이상 (특정 권장 버전 명시 가능)
BLE 플러그인: BleWinrtDll (https://github.com/adabru/BleWinrtDll 에서 다운로드 필요)
BLE 지원 장치: 테스트할 BLE 주변 장치가 필요합니다.
개발 환경:
Windows: Windows 10 이상, Bluetooth 기능 활성화
설치 단계:

GitHub에서 이 저장소를 클론합니다.
Bash

git clone https://github.com/srrck/BleUnityDemo.git
Unity Hub를 사용하여 클론한 프로젝트를 엽니다.
BleWinrtDll 플러그인 통합:
https://github.com/adabru/BleWinrtDll 페이지에서 BleWinrtDll.dll 파일을 다운로드합니다.
Unity 프로젝트의 Assets 폴더 아래에 Plugins 폴더를 생성합니다 (이미 있다면 생략).
다운로드한 BleWinrtDll.dll 파일을 Assets/Plugins 폴더로 복사합니다.
Unity 에디터에서 BleWinrtDll.dll 파일을 선택하고, Inspector 창에서 Platform settings를 Windows로 설정하고, x86_64 또는 Any Platform을 체크합니다. Apply 버튼을 눌러 설정을 적용합니다.
플랫폼 설정: Unity 프로젝트 설정을 Windows로 변경합니다 (File -> Build Settings...). Target Platform을 Windows로 선택하고, Architecture를 사용하시는 환경에 맞게 설정합니다 (x86, x86_64).
추가 설정 (필요시): BleWinrtDll 사용에 필요한 추가적인 설정이 있다면 해당 프로젝트의 문서를 참고하십시오.
사용 방법
프로젝트를 실행하고 BLE 기능을 사용하기 위한 기본적인 단계를 설명합니다.

BLE 매니저 스크립트: 프로젝트 내에 BLE 기능을 관리하는 주요 스크립트가 있을 것입니다. (예: BLEManager.cs). 이 스크립트를 Unity 씬의 적절한 게임 오브젝트에 추가합니다. 이 스크립트는 BleWinrtDll을 사용하여 Windows BLE 기능을 제어하도록 구현되어 있어야 합니다.
UI 요소 연결: BLE 장치 목록, 연결 상태, 데이터 표시 등을 위한 UI 요소 (Text, Button 등)를 생성하고, BLE 매니저 스크립트의 public 변수에 연결합니다.
BLE 기능 활성화: 애플리케이션을 실행하고, BLE 기능을 활성화하는 버튼이나 로직을 트리거합니다.
장치 검색: 주변의 BLE 장치를 검색하는 기능을 실행합니다. 검색된 장치 목록이 UI에 표시될 것입니다.
장치 연결: 목록에서 원하는 BLE 장치를 선택하고 연결을 시도합니다. 연결 상태가 UI에 표시됩니다.
서비스 및 특성 검색: 연결된 장치의 서비스 및 특성을 검색하는 기능을 실행합니다.
데이터 송수신:
읽기: 특정 특성의 값을 읽는 기능을 실행하고, 결과를 UI에 표시합니다.
쓰기: 특정 특성에 데이터를 쓰는 기능을 실행합니다. (데이터 형식 및 전송 방식 설명 필요)
알림 수신: 특정 특성의 값 변경에 대한 알림 기능을 활성화하고, 수신되는 데이터를 UI에 표시합니다.
연결 해제: 연결된 BLE 장치와의 연결을 해제하는 기능을 실행합니다.
