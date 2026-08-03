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

	// Player/Enemy/UIManager가 CreateBullet/CreateEffect/GetPlayer 등
	// GameScene 전용 기능을 바로 쓸 수 있도록, 일부러 Scene*이 아니라 GameScene*을 돌려준다.
	class GameScene* GetScene() const;
	

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

};

