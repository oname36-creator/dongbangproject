#include "pch.h"
#include "LoadingScene.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "ResourceManager.h"
#include "Texture.h"

void LoadingScene::Init()
{
	ResourceManager::GetInstance().LoadTexture(L"LoadingLogo", L"LoadingLogo.bmp", -1);

	fs::path fontPath = ResourceManager::GetInstance().GetResourcePath() / L"Fonts/Griun_Fromsol-Rg.ttf";
	AddFontResourceExW(fontPath.c_str(), FR_PRIVATE, 0);
	_font = CreateFont(-24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"Griun Fromsol");
}

void LoadingScene::Cleanup()
{
	if (_font)
	{
		DeleteObject(_font);
		_font = nullptr;
	}

	fs::path fontPath = ResourceManager::GetInstance().GetResourcePath() / L"Fonts/Griun_Fromsol-Rg.ttf";
	RemoveFontResourceExW(fontPath.c_str(), FR_PRIVATE, 0);
}

void LoadingScene::Update(float deltaTime)
{
	_elapsed += deltaTime;
	if (_elapsed >= 3.0f)
	{
		SceneManager::GetInstance().ChangeScene(new TitleScene());
	}
}

void LoadingScene::Render(HDC hdc)
{
	RECT rect{ 0, 0, GWindowSizeX, GWinSizeY };
	FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

	Texture* logo = ResourceManager::GetInstance().GetTexture(L"LoadingLogo");
	if (logo)
	{
		// 가로를 화면 폭에 맞추고 세로는 비율 유지 (위아래 여백 허용)
		float destSizeX = static_cast<float>(GWindowSizeX);
		float destSizeY = destSizeX * (logo->GetSizeY() / static_cast<float>(logo->GetSizeX()));
		logo->RenderScreen(hdc, Vector(GWindowSizeX / 2.0f, GWinSizeY / 2.0f), Vector(0, 0), Vector(destSizeX, destSizeY));
	}

	HFONT prevFont = _font ? (HFONT)SelectObject(hdc, _font) : nullptr;
	const wchar_t* loading = L"소녀 기도 중...";
	::TextOut(hdc, GWindowSizeX / 2 - 60, GWinSizeY / 2 - 20, loading, static_cast<int32>(wcslen(loading)));
	if (prevFont)
	{
		SelectObject(hdc, prevFont);
	}
}
