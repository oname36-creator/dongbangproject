#pragma once

#include "Scene.h"

enum class TitleMenuItem
{
	CharacterSelect,
	ExtraStage,	// 게임을 1번 이상 클리어해야 메뉴에 나타난다 (SaveManager::IsExtraUnlocked)
	Settings,
	Exit
};

// 타이틀 화면. 배경 + 메뉴(캐릭터 선택/[엑스트라]/설정/종료) + Z 입력으로 확정.
class TitleScene : public Scene
{
public:
	virtual void Init() override;
	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;
	virtual void Cleanup() override;

private:
	HFONT _titleFont = nullptr;
	HFONT _menuFont = nullptr;

	// 언락 상태에 따라 필터링된, 실제로 화면에 보이는 메뉴 목록.
	vector<TitleMenuItem> _menuItems;
	int32 _selectedIndex = 0;
	bool _menuOpen = false;

	// 설정 화면: 0=BGM 음량, 1=효과음 음량, 2=뒤로가기.
	bool _inSettings = false;
	int32 _settingsIndex = 0;
};
