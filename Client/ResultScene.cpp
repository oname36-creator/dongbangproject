#include "pch.h"
#include "ResultScene.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "TitleScene.h"

void ResultScene::Init()
{
}

void ResultScene::Update(float deltaTime)
{
	if (InputManager::GetInstance().GetButtonDown(KeyType::ATTACK))
	{
		SceneManager::GetInstance().ChangeScene(new TitleScene());
	}
}

void ResultScene::Render(HDC hdc)
{
	RECT rect{ 0, 0, GWinSizeX, GWinSizeY };
	FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

	wstring scoreStr = std::format(L"Score: {0}", _score);
	::TextOut(hdc, GWinSizeX / 2 - 60, GWinSizeY / 2 - 20, scoreStr.c_str(), static_cast<int32>(scoreStr.size()));

	const wchar_t* guide = L"Press Z to Title";
	::TextOut(hdc, GWinSizeX / 2 - 60, GWinSizeY / 2 + 10, guide, static_cast<int32>(wcslen(guide)));
}
