#include "pch.h"
#include "TitleScene.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "GameScene.h"

void TitleScene::Init()
{
}

void TitleScene::Update(float deltaTime)
{
	if (InputManager::GetInstance().GetButtonDown(KeyType::ATTACK))
	{
		SceneManager::GetInstance().ChangeScene(new GameScene());
	}
}

void TitleScene::Render(HDC hdc)
{
	RECT rect{ 0, 0, GWinSizeX, GWinSizeY };
	FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

	const wchar_t* title = L"DongbangProject";
	::TextOut(hdc, GWinSizeX / 2 - 60, GWinSizeY / 2 - 20, title, static_cast<int32>(wcslen(title)));

	const wchar_t* guide = L"Press Z to Start";
	::TextOut(hdc, GWinSizeX / 2 - 60, GWinSizeY / 2 + 10, guide, static_cast<int32>(wcslen(guide)));
}
