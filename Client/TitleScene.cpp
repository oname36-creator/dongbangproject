#include "pch.h"
#include "TitleScene.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "CharacterSelectScene.h"
#include "ResourceManager.h"
#include "Texture.h"

void TitleScene::Init()
{
	ResourceManager::GetInstance().LoadTexture(L"TitleLogo", L"TitleLogo.bmp", -1);

	fs::path fontPath = ResourceManager::GetInstance().GetResourcePath() / L"Fonts/Griun_Fromsol-Rg.ttf";
	AddFontResourceExW(fontPath.c_str(), FR_PRIVATE, 0);
	_titleFont = CreateFont(-44, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"Griun Fromsol");
}

void TitleScene::Cleanup()
{
	if (_titleFont)
	{
		DeleteObject(_titleFont);
		_titleFont = nullptr;
	}

	fs::path fontPath = ResourceManager::GetInstance().GetResourcePath() / L"Fonts/Griun_Fromsol-Rg.ttf";
	RemoveFontResourceExW(fontPath.c_str(), FR_PRIVATE, 0);
}

void TitleScene::Update(float deltaTime)
{
	if (!_menuOpen)
	{
		if (InputManager::GetInstance().GetButtonDown(KeyType::ATTACK))
		{
			_menuOpen = true;
		}
		return;
	}

	if (InputManager::GetInstance().GetButtonDown(KeyType::Up))
	{
		int32 index = static_cast<int32>(_selected) - 1;
		if (index < 0)
		{
			index = static_cast<int32>(TitleMenuItem::Count) - 1;
		}
		_selected = static_cast<TitleMenuItem>(index);
	}

	if (InputManager::GetInstance().GetButtonDown(KeyType::Down))
	{
		int32 index = static_cast<int32>(_selected) + 1;
		if (index >= static_cast<int32>(TitleMenuItem::Count))
		{
			index = 0;
		}
		_selected = static_cast<TitleMenuItem>(index);
	}

	if (InputManager::GetInstance().GetButtonDown(KeyType::ATTACK))
	{
		switch (_selected)
		{
		case TitleMenuItem::CharacterSelect:
			SceneManager::GetInstance().ChangeScene(new CharacterSelectScene());
			break;
		case TitleMenuItem::Settings:
			// 사운드 시스템이 아직 없어 자리표시만 유지 (선택해도 동작 없음)
			break;
		case TitleMenuItem::Exit:
			PostQuitMessage(0);
			break;
		}
	}
}

void TitleScene::Render(HDC hdc)
{
	RECT rect{ 0, 0, GWindowSizeX, GWinSizeY };
	FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

	Texture* logo = ResourceManager::GetInstance().GetTexture(L"TitleLogo");
	if (logo)
	{
		// 가로를 화면 폭에 맞추고 세로는 비율 유지 (위아래 여백 허용)
		float destSizeX = static_cast<float>(GWindowSizeX);
		float destSizeY = destSizeX * (logo->GetSizeY() / static_cast<float>(logo->GetSizeX()));
		logo->RenderScreen(hdc, Vector(GWindowSizeX / 2.0f, GWinSizeY / 2.0f), Vector(0, 0), Vector(destSizeX, destSizeY));
	}

	if (_titleFont)
	{
		HFONT prevFont = (HFONT)SelectObject(hdc, _titleFont);
		COLORREF prevColor = SetTextColor(hdc, RGB(255, 255, 255));
		int32 prevBkMode = SetBkMode(hdc, TRANSPARENT);

		const wchar_t* titleChars = L"동방홍마향";
		constexpr int32 charCount = 5;
		constexpr int32 startX = 60;
		constexpr int32 startY = 150;
		constexpr int32 lineHeight = 56;
		for (int32 i = 0; i < charCount; ++i)
		{
			::TextOut(hdc, startX, startY + i * lineHeight, titleChars + i, 1);
		}

		SetTextColor(hdc, prevColor);
		SetBkMode(hdc, prevBkMode);
		SelectObject(hdc, prevFont);

		const wchar_t* subtitle = L"(모작)";
		::TextOut(hdc, startX - 10, startY + charCount * lineHeight + 5, subtitle, static_cast<int32>(wcslen(subtitle)));
	}

	if (!_menuOpen)
	{
		const wchar_t* guide = L"Press Z to Start";
		::TextOut(hdc, GWindowSizeX / 2 - 60, GWinSizeY - 100, guide, static_cast<int32>(wcslen(guide)));
	}
	else
	{
		const wchar_t* menuLabels[] = { L"캐릭터 선택", L"설정", L"게임 종료" };
		constexpr int32 menuStartX = GWindowSizeX - 220;
		constexpr int32 menuStartY = 250;
		constexpr int32 menuLineHeight = 40;
		for (int32 i = 0; i < static_cast<int32>(TitleMenuItem::Count); ++i)
		{
			wstring line = (static_cast<int32>(_selected) == i ? L"> " : L"  ") + wstring(menuLabels[i]);
			::TextOut(hdc, menuStartX, menuStartY + i * menuLineHeight, line.c_str(), static_cast<int32>(line.size()));
		}
	}
}
