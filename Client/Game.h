#pragma once

#include "Singleton.h"


// 전체 게임 로직을 담당하는 클래스
class Game : public Singleton<Game>
{
	// Singleton 객체를 '친구'로 선언해서 private 접근 가능하게 열어준다.
	friend Singleton<Game>;

public:
	void Init(HWND hwnd);
	void Cleanup();

	void Update();
	void Render();

	HWND GetHwnd() const { return _hwnd; }

	// TODO(1주차 Day1~2): SceneManager 도입 시 이 함수를 어떻게 할지 결정할 것
	//  Player, Enemy, UIManager가 전부 Game::GetInstance().GetScene()을 통해 씬에 접근한다.
	//  씬이 여러 개가 되면 '현재 씬'을 돌려주는 SceneManager::GetCurrentScene()으로 옮기고,
	//  GameScene 고유 기능(CreateBullet 등)을 쓰려면 GameScene*으로 캐스팅이 필요해진다.
	//  → 호출하는 쪽을 전부 고쳐야 하니, 씬 분리 작업과 '한 번에' 처리해라.
	class Scene* GetScene() const { return _scene; }

private:
	// 아무나 생성못하게 생성자/소멸자를 숨기자
	Game() = default;
	~Game() = default;

private:
	HWND _hwnd;	// 윈도우 핸들
	RECT _rect;		// 윈도우 크기
	
	HDC _hdc;		// 메인 도화지 (출력용)
	HDC _hdcBack;	// 실시간으로 그려지는 버퍼
	HBITMAP _bmpBack;	// back hdc가 사용하는 텍스처

	class Scene* _scene = nullptr;
};

