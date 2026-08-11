#include "pch.h"
#include "EndingScene.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "ResourceManager.h"
#include "Texture.h"

void EndingScene::Init()
{
	ResourceManager::GetInstance().LoadTexture(L"StageResultBG", L"StageResultBG.bmp", -1);
}

void EndingScene::Update(float deltaTime)
{
	if (InputManager::GetInstance().GetButtonDown(KeyType::ATTACK))
	{
		SceneManager::GetInstance().ChangeScene(new TitleScene());
	}
}

void EndingScene::Render(HDC hdc)
{
	RECT rect{ 0, 0, GWindowSizeX, GWinSizeY };
	FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

	Texture* bg = ResourceManager::GetInstance().GetTexture(L"StageResultBG");
	if (bg)
	{
		bg->RenderScreen(hdc, Vector(GWindowSizeX / 2.0f, GWinSizeY / 2.0f), Vector(0, 0), Vector((float)GWindowSizeX, (float)GWinSizeY));
	}

	const wchar_t* congrats = L"Congratulations!";
	::TextOut(hdc, GWindowSizeX / 2 - 70, GWinSizeY / 2 - 50, congrats, static_cast<int32>(wcslen(congrats)));

	wstring scoreStr = std::format(L"Score: {0}", _score);
	::TextOut(hdc, GWindowSizeX / 2 - 60, GWinSizeY / 2 - 20, scoreStr.c_str(), static_cast<int32>(scoreStr.size()));

	const wchar_t* guide = L"Press Z to Title";
	::TextOut(hdc, GWindowSizeX / 2 - 60, GWinSizeY / 2 + 10, guide, static_cast<int32>(wcslen(guide)));
}
