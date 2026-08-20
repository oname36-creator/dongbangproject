#include "pch.h"
#include "ResultScene.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "ResourceManager.h"

void ResultScene::Init()
{
	fs::path fontPath = ResourceManager::GetInstance().GetResourcePath() / L"Fonts/Griun_Fromsol-Rg.ttf";
	AddFontResourceExW(fontPath.c_str(), FR_PRIVATE, 0);
	_font = CreateFont(-28, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"Griun Fromsol");
}

void ResultScene::Cleanup()
{
	if (_font)
	{
		DeleteObject(_font);
		_font = nullptr;
	}

	fs::path fontPath = ResourceManager::GetInstance().GetResourcePath() / L"Fonts/Griun_Fromsol-Rg.ttf";
	RemoveFontResourceExW(fontPath.c_str(), FR_PRIVATE, 0);
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
	RECT rect{ 0, 0, GWindowSizeX, GWinSizeY };
	FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

	HFONT prevFont = _font ? (HFONT)SelectObject(hdc, _font) : nullptr;
	wstring scoreStr = std::format(L"점수 : {0}", _score);
	::TextOut(hdc, GWindowSizeX / 2 - 60, GWinSizeY / 2 - 20, scoreStr.c_str(), static_cast<int32>(scoreStr.size()));
	if (prevFont)
	{
		SelectObject(hdc, prevFont);
	}

	const wchar_t* guide = L"Press Z to Title";
	::TextOut(hdc, GWindowSizeX / 2 - 60, GWinSizeY / 2 + 10, guide, static_cast<int32>(wcslen(guide)));
}
