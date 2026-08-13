#include "pch.h"
#include "Game.h"
#include "TimeManager.h"
#include "InputManager.h"
#include "Util.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "GameScene.h"
#include "CollisionManager.h"
#include "DataManager.h"
#include "UIManager.h"
#include "SceneManager.h"
#include "LoadingScene.h"
#include "AudioManager.h"
#include "SaveManager.h"

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
	wchar_t buffer[MAX_PATH];
	DWORD length = ::GetCurrentDirectory(MAX_PATH, buffer);
	fs::path currentPath = fs::path(buffer) / L"Resources/";
	ResourceManager::GetInstance().Init(hwnd, currentPath);

	// DataManager 초기화
	DataManager::GetInstance().Init(currentPath);
	DataManager::GetInstance().Load();

	// SaveManager 초기화 (재실행해도 유지되는 진행 상황 로드)
	SaveManager::GetInstance().Init(currentPath);

	// AudioManager 초기화
	AudioManager::GetInstance().Init(currentPath);
	AudioManager::GetInstance().LoadSound(L"Fire", L"Fire.wav");
	AudioManager::GetInstance().LoadSound(L"Hit", L"Hit.wav");
	AudioManager::GetInstance().LoadSound(L"Explosion", L"Explosion.wav");
	AudioManager::GetInstance().LoadSound(L"TitleBGM", L"TitleBGM.wav");
	AudioManager::GetInstance().LoadSound(L"TitleSelect", L"TitleSelect.wav");
	AudioManager::GetInstance().LoadSound(L"Stage1WaveBGM", L"Stage1WaveBGM.wav");
	AudioManager::GetInstance().LoadSound(L"Stage1BossBGM", L"Stage1BossBGM.wav");
	AudioManager::GetInstance().LoadSound(L"Stage2WaveBGM", L"Stage2WaveBGM.wav");
	AudioManager::GetInstance().LoadSound(L"Stage2BossBGM", L"Stage2BossBGM.wav");
	AudioManager::GetInstance().LoadSound(L"Stage3WaveBGM", L"Stage3WaveBGM.wav");
	AudioManager::GetInstance().LoadSound(L"Stage3MidBossBGM", L"Stage3MidBossBGM.wav");
	AudioManager::GetInstance().LoadSound(L"Stage3BossBGM", L"Stage3BossBGM.wav");
	AudioManager::GetInstance().LoadSound(L"ExtraBGM", L"ExtraBGM.wav");

	// GameScene 초기화
	SceneManager::GetInstance().ChangeScene(new LoadingScene());

	// CollisionManager 초기화
	CollisionManager::GetInstance().Init();
	UIManager::GetInstance().Init();
}

void Game::Cleanup()
{
	SceneManager::GetInstance().Cleanup();

	// 매니저들 각자 정리가 필요한것들은 정리해준다.
	ResourceManager::GetInstance().Cleanup();
	AudioManager::GetInstance().Cleanup();
}

void Game::Update()
{
	// 각종 업데이트 로직 처리
	TimeManager::GetInstance().Update();

	// 입력 업데이트 
	InputManager::GetInstance().Update();

	// Scene 업데이트
	SceneManager::GetInstance().Update(TimeManager :: GetInstance().GetDT());

	// 모든 Update가 끝나고 좌표 갱신이 완료된 후, 충돌체크 수행
	UIManager::GetInstance().Update(TimeManager::GetInstance().GetDT());
	CollisionManager::GetInstance().Update();

	// 재생이 끝난 소스 보이스 정리
	AudioManager::GetInstance().Update();
}

void Game::Render()
{
	// 각종 렌더링 로직 처리
	// Scene의 모든 객체 렌더링
	SceneManager::GetInstance().Render(_hdcBack);

	CollisionManager::GetInstance().Render(_hdcBack);

	UIManager::GetInstance().Render(_hdcBack);

	// UI(사이드바)보다 나중에 그려야 하는 오버레이(스테이지 결과 화면 등) — UI 위까지 덮는다.
	if (Scene* curScene = SceneManager::GetInstance().GetCurrentScene())
	{
		curScene->RenderOverlay(_hdcBack);
	}

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
class GameScene* Game::GetScene() const
{
	return dynamic_cast<GameScene*>(SceneManager::GetInstance().GetCurrentScene());
}

