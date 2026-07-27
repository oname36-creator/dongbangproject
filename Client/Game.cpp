#include "pch.h"
#include "Game.h"
#include "TimeManager.h"
#include "InputManager.h"
#include "Util.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "Scene.h"
#include "CollisionManager.h"
#include "DataManager.h"
#include "UIManager.h"

void Game::Init(HWND hwnd)
{
	_hwnd = hwnd;

	// 해당 윈도우가 그려지는 메인 HDC 도화지를 얻어오기
	_hdc = GetDC(hwnd);

	// 더블버퍼링
	_hdcBack = CreateCompatibleDC(_hdc);

	// 윈도우 크기를 가져온다. (출력용 도화지와 크기를 맞추기 위해서)
	GetClientRect(hwnd, &_rect);

	// 더블버퍼링을 위한 HDC는 생성, HDC안에는 아주 작은 크기의 텍스처가 할당되어있다.
	// 게임 크기와 맞는 텍스처를 생성해서, BackBuffer에 설정한다.
	_bmpBack = CreateCompatibleBitmap(_hdc, _rect.right, _rect.bottom);

	// 생성된 백버퍼 HDC에 맞는 텍스처를 연결한다.
	HBITMAP prev = (HBITMAP)SelectObject(_hdcBack, _bmpBack);
	DeleteObject(prev);	// 기존에 가지고있던 작은 텍스처는 버린다.

	
	// TimeManager
	TimeManager::GetInstance().Init();

	// InputManager
	InputManager::GetInstance().Init(hwnd);

	// full path c:// /// // /
	// 리소스 매니저 초기화
	// TODO(1주차 Day0): 리소스 경로 고정 ★ 다른 무엇보다 먼저 할 일
	//  문제: GetCurrentDirectory()는 '실행 시점의 작업 디렉터리'를 돌려준다.
	//        VS 디버거로 실행하면 작업 디렉터리 기본값이 $(ProjectDir) = 레포 루트인데,
	//        거기에 "../Resources/"를 붙이면 레포 '바깥'을 가리키게 된다 → 리소스 로드 실패.
	//  할 일: 아래 L"../Resources/" 를 L"Resources/" 로 바꾸고,
	//        프로젝트 속성 > 디버깅 > 작업 디렉터리가 $(ProjectDir)인지 확인한다.
	//  검증: 실행했을 때 "Failed to open JSON file" 메시지박스가 안 뜨면 성공.
	//  참고: 이 경로 하나를 ResourceManager와 DataManager가 함께 쓴다(바로 아래 줄들).
	//        그리고 Resources/ 폴더가 지금 비어 있다 → 플레이스홀더 bmp와
	//        Resources/Data/ResourceData.json 을 먼저 채워야 화면에 뭐라도 나온다.
	wchar_t buffer[MAX_PATH];
	DWORD length = ::GetCurrentDirectory(MAX_PATH, buffer);
	fs::path currentPath = fs::path(buffer) / L"../Resources/";
	ResourceManager::GetInstance().Init(hwnd, currentPath);

	// DataManager 초기화
	DataManager::GetInstance().Init(currentPath);
	DataManager::GetInstance().Load();

	// Scene 초기화
	// TODO(1주차 Day1~2): Game이 씬을 직접 소유하지 않도록 구조를 바꿀 것
	//  현재: Game이 Scene 1개를 new로 만들어 끝까지 들고 있다 → 타이틀/결과 화면으로 넘어갈 수 없다.
	//  목표(기획서 3장): TitleScene / GameScene / ResultScene 전환.
	//  할 일:
	//   1) Scene을 추상 베이스로 만든다 (virtual ~Scene(), virtual Init/Cleanup/Update/Render)
	//   2) 지금 Scene의 내용을 GameScene으로 옮기고, TitleScene/ResultScene은 껍데기로 추가한다
	//      (배경 + 안내 문구 + 키 입력 하나면 충분하다. 여기서 시간 쓰지 말 것)
	//   3) Engine/ 에 SceneManager 싱글톤을 만들고, 아래 2줄과 Cleanup/Update/Render의
	//      _scene 접근을 전부 SceneManager 호출로 교체한다
	//  함정: 씬 교체를 '즉시' 하면 안 된다. Update() 도중에 현재 씬을 delete하면
	//        호출 스택이 이미 죽은 객체 위에서 계속 돌아 크래시한다.
	//        → '예약해두고 프레임 끝에서 교체'하는 방식을 써라.
	//        이 프로젝트에 이미 같은 패턴이 있다: Scene::_reservedAdd / _reservedRemove 를 참고.
	_scene = new Scene();
	_scene->Init();

	// CollisionManager 초기화
	CollisionManager::GetInstance().Init();
	UIManager::GetInstance().Init();
}

void Game::Cleanup()
{
	_scene->Cleanup();
	delete _scene;
	_scene = nullptr;

	// 매니저들 각자 정리가 필요한것들은 정리해준다.
	ResourceManager::GetInstance().Cleanup();
}

void Game::Update()
{
	// 각종 업데이트 로직 처리
	TimeManager::GetInstance().Update();

	// 입력 업데이트 
	InputManager::GetInstance().Update();

	// Scene 업데이트
	if (_scene)
	{
		_scene->Update(TimeManager::GetInstance().GetDT());
	}

	// 모든 Update가 끝나고 좌표 갱신이 완료된 후, 충돌체크 수행
	UIManager::GetInstance().Update(TimeManager::GetInstance().GetDT());
	CollisionManager::GetInstance().Update();
}

void Game::Render()
{
	// 각종 렌더링 로직 처리
	// Scene의 모든 객체 렌더링
	if (_scene)
	{
		_scene->Render(_hdcBack);
	}

	CollisionManager::GetInstance().Render(_hdcBack);

	UIManager::GetInstance().Render(_hdcBack);

	// 현재 FPS 를 출력
	{
		wstring str = std::format(L"FPS({0})", TimeManager::GetInstance().GetFPS());
		::TextOut(_hdcBack, 5, 10, str.c_str(), static_cast<int32>(str.size()));
	}

	// 모든 객체들이 백퍼에 그림을 다 그렸다.
	// 함수 끝나기 직전->모든 렌더링이 다 끝났다
	BitBlt(_hdc, 0, 0, _rect.right, _rect.bottom, _hdcBack, 0, 0, SRCCOPY);

	PatBlt(_hdcBack, 0, 0, _rect.right, _rect.bottom, WHITENESS);
}


