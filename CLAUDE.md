# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

WinAPI(Win32 GDI) 기반 2D 종스크롤 슈팅 게임으로 동방프로젝트(Touhou Project) 모작을 목표로 한다 (`README.md`). 교수님이 제공한 "1945" 슈팅 예제를 기반으로 이식했고, 현재 게임 로직(Player/Enemy/Bullet 등)은 아직 원본 예제 그대로이며 동방 컨셉으로의 전환은 진행 전 단계다. 엔트리포인트 파일명 등 원본의 "Game1945" 네이밍은 `DongbangProject`로 모두 교체했다.

## Build

- Visual Studio 2022, `PlatformToolset v143`, C++20 (`stdcpp20`), `RootNamespace`/실행 파일명은 `DongbangProject`.
- 솔루션은 레포 루트의 `DongbangProject.sln` → `DongbangProject.vcxproj`.
- 커맨드라인 빌드:
  ```
  & "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" DongbangProject.sln /p:Configuration=Debug /p:Platform=x64
  ```
  (`Configuration`: Debug/Release, `Platform`: x64/Win32 — Win32 구성은 실제로 빌드해본 적 없음)
- 산출물은 `x64/Debug/DongbangProject.exe` 등 솔루션 기준 경로에 생성됨(빌드 시 자동 생성되는 디렉터리이며 `.gitignore`에서 제외됨).
- 테스트 프레임워크나 린트 설정은 없음 — 임의로 새로 만들지 말 것.
- 주의: PCH(`pch.h`/`pch.cpp`) 설정은 `Debug|x64` 구성에만 `<PrecompiledHeader>Use/Create</PrecompiledHeader>`가 걸려 있고 나머지 구성(Release, Win32)에는 없음. 원본 템플릿부터 있던 상태이니 임의로 "통일"하지 말 것.

## 폴더 구조 (Engine / Client / Common)

레포 루트에는 `DongbangProject.sln`/`.vcxproj`/`.vcxproj.filters`, `.gitignore`, `README.md`, `CLAUDE.md`, `Resources/`(에셋 placeholder)만 있고, 소스는 전부 아래 세 폴더 밑에 있다.

- `Engine/` — 재사용 가능한 프레임워크 코드. `Actor`(모든 게임 오브젝트의 기반 클래스), `Component`(Actor에 붙는 부가 기능: `ImageRenderer`/`SpriteAnimRenderer`/`ColliderCircle`), `ObjectPool<T>`, `CollisionManager`, `ResourceManager`/`Texture`, `TimeManager`, `InputManager`, `DataManager`/`DataObject`, `Singleton<T>`.
- `Client/` — 이 게임 고유의 콘텐츠/로직 + Win32 엔트리포인트. `DongbangProject.cpp/h/.rc/.ico`, `small.ico`(WinMain, 윈도우 생성, 메시지 루프, 리소스 스크립트 — 원본 "Game1945" 네이밍에서 전부 교체됨), `Game`(최상위 오케스트레이터), `Scene`(플레이 필드의 모든 Actor 소유), `Player`/`Enemy`(둘 다 `Airplane` 상속), `Bullet`, `Effect`, `Background`/`WorldBG`, `UIManager`, `ResourceData`(이 게임 전용 에셋 JSON 스키마). **동방프로젝트 컨셉 작업(캐릭터, 탄막 패턴 등)은 주로 이 폴더에서 진행하게 된다.**
- `Common/` — Engine/Client 양쪽이 참조하는 플랫폼/빌드 인프라 + 공용 유틸. `pch.h/cpp`(미리 컴파일된 헤더 — Debug|x64에서만 사용됨, 아래 참고), `framework.h`/`targetver.h`/`Resource.h`(Win32 보일러플레이트, 리소스 ID 정의), `Util.h`(Vector 구조체, `RenderLayer`/`ActorType`/`BulletType` enum, `GWinSizeX`/`GWinSizeY` 상수, `int8`~`uint64` 타입 별칭), `Json/nlohmann`(벤더링된 nlohmann/json 싱글헤더).
- 이 분리는 물리적 폴더 이동만 한 것이고 소스의 `#include "Actor.h"` 같은 구문은 손대지 않았다. 대신 `DongbangProject.vcxproj`의 `ClCompile`/`ResourceCompile` 양쪽 `AdditionalIncludeDirectories`에 프로젝트 루트/`Engine`/`Client`/`Common`이 모두 추가되어 있어서, 파일이 실제로 어느 폴더에 있든 quoted include(`#include "pch.h"` 등, `.rc`의 `#include "resource.h"` 포함)가 해결된다. 새 최상위 폴더를 추가한다면 두 `AdditionalIncludeDirectories` 목록도 같이 업데이트해야 한다.
- 리소스 스크립트/식별자 리네이밍: `Common/Resource.h`의 매크로(`IDI_DONGBANGPROJECT`, `IDC_DONGBANGPROJECT`, `IDD_DONGBANGPROJECT_DIALOG`)와 `Client/DongbangProject.rc`의 문자열 테이블·다이얼로그 캡션은 전부 `DongbangProject`로 통일되어 있다. 새 아이콘/문자열을 추가할 때 원본 "Game1945" 이름이 다시 섞여 들어가지 않도록 주의.

## 실행 시 필요한 리소스

- `Resources/`, `Resources/Data/`는 현재 빈 폴더(placeholder)만 있고 실제 이미지(.bmp)나 JSON은 아직 커밋되어 있지 않다.
- `Game.cpp`의 `Game::Init()`이 `GetCurrentDirectory() / "../Resources/"` 를 계산해서 `ResourceManager`와 `DataManager` 양쪽에 넘긴다.
- `DataManager::Load()`는 `Resources/Data/ResourceData.json`을 읽어야 하며, 스키마는 다음과 같다 (`ResourceData::Load` 참고):
  ```json
  { "GameScene": {
      "<텍스처 키>": {
        "fileName": "Player.bmp",
        "transparent": [r, g, b],
        "countX": 1, "countY": 1,
        "loop": false, "dur": 1.0
      }
  }}
  ```
  `Scene::loadResources()`는 이 JSON을 통해서만 텍스처를 로드한다(과거 하드코딩된 `LoadTexture(...)` 호출들은 주석 처리되어 남아있을 뿐 더 이상 쓰이지 않음). 즉 이 JSON과 대응하는 bmp들이 없으면 실행 시 "Failed to open JSON file" 메시지박스가 뜨고 아무 것도 그려지지 않는다.
- 주의: `../Resources/`는 실행 시점의 프로세스 작업 디렉터리 기준이다. 비주얼 스튜디오 디버거로 실행하면 작업 디렉터리 기본값은 `$(ProjectDir)`인데, `vcxproj`가 레포 루트에 있으므로 이는 레포 루트 자신이 되고, 결과적으로 `../Resources`는 **레포 바깥의 상위 폴더**를 가리키게 된다. 리소스가 로드되지 않으면 디버깅 속성의 Working Directory(또는 실행 시 cwd)부터 확인할 것.

## 아키텍처

### 코어 루프
- `Client/DongbangProject.cpp`의 `wWinMain`은 블로킹 `GetMessage` 대신 커스텀 루프를 돈다: 메시지가 있으면 `PeekMessage`로 처리하고, 없으면 `QueryPerformanceCounter`로 경과 시간을 재서 120FPS 프레임 예산을 넘을 때만 `Game::Update()`/`Game::Render()`를 호출한다.
- `Game`(싱글톤)이 더블 버퍼링용 백버퍼(HDC/HBITMAP)와 단일 `Scene` 인스턴스를 소유한다. `Game::Init()`이 `ResourceManager`/`DataManager`/`CollisionManager`/`UIManager`를 초기화하고 `Scene::Init()`을 호출하는 진입점.
- `Scene`이 모든 Actor를 `vector<Actor*> _actors`로 소유하고, `RenderLayer`별 렌더 리스트(`_renderList[RenderLayer]`)와 공간 분할용 균일 그리드(`_grid`, 셀 크기 50)를 함께 관리한다. Actor 추가/삭제는 `_reservedAdd`/`_reservedRemove`에 모아뒀다가 `Scene::Update()` 한 번에 반영한다 — 순회 중 `_actors`를 직접 변경하지 않기 위함.
- `Bullet`과 `Enemy`는 `ObjectPool<T>`에서 나온다(초기화 시 고정 크기로 미리 할당, 런타임에 늘어나지 않고 `Acquire()`가 고갈 시 `assert`). 나머지(`Player`, `Effect`, `Background`/`WorldBG`)는 `new`로 생성되고, 소유 풀이 없는 Actor(`Actor::GetPool() == nullptr`)에 한해 `Scene::Cleanup()`이 `delete`한다.

### Actor/Component 모델
- `Actor`가 기본 게임 오브젝트: 위치, pending-kill 플래그, 소속 `ObjectPool` 포인터(풀 소속이 아니면 nullptr), `AddComponent<T>()`/`GetComponent<T>()`(선형 탐색 + `dynamic_cast`)로 확장하는 `vector<Component*>`.
- 실제 Actor들(`Airplane` → `Player`/`Enemy`, `Bullet`, `Effect`, `Background`/`WorldBG`)은 Component 조합이 아니라 `Actor`를 직접 상속해서 만든다. Component는 그리기(`ImageRenderer`/`SpriteAnimRenderer`)나 충돌(`ColliderCircle`) 같은 부가 기능에만 쓰인다.
- 모든 구체 Actor는 `GetRenderLayer()`/`GetActorType()`(순수가상)을 구현해야 한다. `RenderLayer`는 그리기 순서/렌더 리스트 버킷을 결정하고(`Common/Util.h`의 enum 선언 순서 = Background → Enemy → Bullet → Player → Effect), `ActorType`은 `CollisionManager`의 무시 마스크에 쓰인다.

### 충돌
- `CollisionManager`(싱글톤)는 `Scene::registerActor`/`removeActor`에서 ColliderCircle이 있는 Actor만 등록받는다. 프레임 간 겹침 상태(`_prev`/`_curr`, Actor 쌍의 set)를 비교해 Enter/Stay/Exit 전이를 판정하며, `ActorType` 쌍별 `IGNORE_MASK`로 특정 타입 조합은 아예 무시할 수 있다.

### 리소스 로딩
- `ResourceManager`(싱글톤)는 단순 `key -> Texture*` 맵. `Texture`는 GDI 투명 블릿(빌드 시 `msimg32.lib` 링크)과 선택적 스프라이트시트 row/col + 애니메이션 재생시간을 감싼다.
- `DataManager`(싱글톤)는 `Resources/Data/*.json`에서 `DataObject` 하위클래스(wstring 키로 구분)를 로드한다(nlohmann::json 사용). 현재 구현된 `DataObject`는 `ResourceData` 하나뿐이며, 위 "실행 시 필요한 리소스" 항목의 텍스처 로딩을 담당한다.

## 인코딩

모든 `.h`/`.cpp`/`.rc` 소스 파일은 UTF-8(BOM 포함)로 통일되어 있다(`.vcxproj`/`.vcxproj.filters`/`.sln`은 BOM 없는 순수 UTF-8). 원본 교수님 코드는 CP949와 UTF-8이 섞여 있었으나 전부 UTF-8(BOM)로 재인코딩했고, `Client/DongbangProject.rc`도 원래 UTF-16이던 것을 UTF-8(BOM)로 변환했다(v143 리소스 컴파일러는 BOM 있는 UTF-8 `.rc`를 정상 처리함). 앞으로 파일을 저장할 때 이 인코딩을 유지할 것 — 특히 셸 스크립트나 외부 도구로 일괄 치환할 때 CP949로 되돌아가지 않도록 주의.
