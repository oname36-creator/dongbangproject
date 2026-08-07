#pragma once

#include "Scene.h"

enum class TitleMenuItem
{
	CharacterSelect,
	Settings,
	Exit,
	Count
};

// 타이틀 화면. 배경 + 메뉴(캐릭터 선택/설정/종료) + Z 입력으로 확정.
class TitleScene : public Scene
{
public:
	virtual void Init() override;
	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;
	virtual void Cleanup() override;

private:
	HFONT _titleFont = nullptr;
	TitleMenuItem _selected = TitleMenuItem::CharacterSelect;
	bool _menuOpen = false;
};
