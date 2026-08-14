#pragma once

#include "Singleton.h"

// 키보드&마우스의 입력 처리
// 현재 마우스의 좌표가 뭐지?
// 지금 W 키가 Pressed ? Released 됐나 ?

//VK_SPACE : 이런 기능을 그대로 사용
enum class KeyType
{
	LeftMouse = VK_LBUTTON,
	RightMouse = VK_RBUTTON,

	PAUSE = VK_ESCAPE,

	Up = VK_UP,
	Down = VK_DOWN,
	Left = VK_LEFT,
	Right = VK_RIGHT,
	SpaceBar = VK_SPACE,

	KEY_1 = '1',
	KEY_2 = '2',
	KEY_3 = '3',
	KEY_4 = '4',

	LOW_SPEED = VK_SHIFT,
	ATTACK = 'Z',
	BOOM = 'X',

	W = 'W',
	A = 'A',
	S = 'S',
	D = 'D',
	L = 'L',
	Q = 'Q',
	E = 'E',

	F1 = VK_F1,
	F2 = VK_F2,
	F3 = VK_F3,
};

enum class KeyState
{
	None,
	Press,
	Down,
	Up,

	End
};

constexpr int32 KEY_TYPE_COUNT = static_cast<int32>(UINT8_MAX) + 1;

class InputManager : public Singleton<InputManager>
{
	// Singleton 객체를 '친구'로 선언해서 private 접근 가능하게 열어준다.
	friend Singleton<InputManager>;

public:
	void Init(HWND hwnd);
	void Update();

	// 누르고 있을 때
	bool GetButtonPressed(KeyType key) { return GetState(key) == KeyState::Press; }

	// 맨 처음 눌렀을 때
	bool GetButtonDown(KeyType key) { return GetState(key) == KeyState::Down; }

	// 맨 처음 눌렀다가 땔 때
	bool GetButtonUp(KeyType key) { return GetState(key) == KeyState::Up; }

	// 현재 마우스 위치
	POINT GetMousePos() { return _mousePos; }

private:
	KeyState GetState(KeyType key) { return _states[static_cast<uint8>(key)]; }

private:
	// 아무나 생성못하게 생성자/소멸자를 숨기자
	InputManager() = default;
	~InputManager() = default;


	HWND _hwnd = 0;
	vector<KeyState> _states;
	POINT _mousePos = {};
};

