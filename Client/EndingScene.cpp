#include "pch.h"
#include "EndingScene.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "ResourceManager.h"
#include "Texture.h"

namespace
{
	constexpr float SLIDE_HOLD_DURATION = 2.0f;	// end01/end02가 페이드아웃 전까지 떠있는 시간
	constexpr float SLIDE_FADE_DURATION = 0.8f;	// 검은 화면으로 페이드아웃되는 시간
}

void EndingScene::Init()
{
	ResourceManager::GetInstance().LoadTexture(L"StageResultBG", L"StageResultBG.bmp", -1);
	ResourceManager::GetInstance().LoadTexture(L"EndSlide1", L"end01.bmp", -1);
	ResourceManager::GetInstance().LoadTexture(L"EndSlide2", L"end02.bmp", -1);
	ResourceManager::GetInstance().LoadTexture(L"EndSlide3", L"end06.bmp", -1);

	fs::path fontPath = ResourceManager::GetInstance().GetResourcePath() / L"Fonts/Griun_Fromsol-Rg.ttf";
	AddFontResourceExW(fontPath.c_str(), FR_PRIVATE, 0);
	_font = CreateFont(-28, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"Griun Fromsol");
}

void EndingScene::Cleanup()
{
	if (_font)
	{
		DeleteObject(_font);
		_font = nullptr;
	}

	fs::path fontPath = ResourceManager::GetInstance().GetResourcePath() / L"Fonts/Griun_Fromsol-Rg.ttf";
	RemoveFontResourceExW(fontPath.c_str(), FR_PRIVATE, 0);
}

void EndingScene::Update(float deltaTime)
{
	if (_phase == EndingPhase::Score)
	{
		if (InputManager::GetInstance().GetButtonDown(KeyType::ATTACK))
		{
			_phase = EndingPhase::Illustration;
			_slideIndex = 0;
			_slideTimer = 0.f;
			_fadingOut = false;
		}
		return;
	}

	_slideTimer += deltaTime;

	bool isLastSlide = (_slideIndex >= 2);

	if (!_fadingOut)
	{
		// 마지막 슬라이드(end06)는 시간이 아니라 Z 입력으로 페이드아웃을 시작한다.
		bool shouldFade = isLastSlide
			? InputManager::GetInstance().GetButtonDown(KeyType::ATTACK)
			: (_slideTimer >= SLIDE_HOLD_DURATION);

		if (shouldFade)
		{
			_fadingOut = true;
			_slideTimer = 0.f;
		}
	}
	else if (_slideTimer >= SLIDE_FADE_DURATION)
	{
		_slideIndex++;
		_slideTimer = 0.f;
		_fadingOut = false;

		if (_slideIndex > 2)
		{
			SceneManager::GetInstance().ChangeScene(new TitleScene());
		}
	}
}

void EndingScene::Render(HDC hdc)
{
	RECT rect{ 0, 0, GWindowSizeX, GWinSizeY };
	FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

	if (_phase == EndingPhase::Score)
	{
		Texture* bg = ResourceManager::GetInstance().GetTexture(L"StageResultBG");
		if (bg)
		{
			bg->RenderScreen(hdc, Vector(GWindowSizeX / 2.0f, GWinSizeY / 2.0f), Vector(0, 0), Vector((float)GWindowSizeX, (float)GWinSizeY));
		}

		HFONT prevFont = _font ? (HFONT)SelectObject(hdc, _font) : nullptr;

		const wchar_t* congrats = L"게임 클리어!";
		::TextOut(hdc, GWindowSizeX / 2 - 70, GWinSizeY / 2 - 50, congrats, static_cast<int32>(wcslen(congrats)));

		wstring scoreStr = std::format(L"점수 : {0}", _score);
		::TextOut(hdc, GWindowSizeX / 2 - 60, GWinSizeY / 2 - 20, scoreStr.c_str(), static_cast<int32>(scoreStr.size()));

		if (prevFont)
		{
			SelectObject(hdc, prevFont);
		}

		const wchar_t* guide = L"Press Z to Title";
		::TextOut(hdc, GWindowSizeX / 2 - 60, GWinSizeY / 2 + 10, guide, static_cast<int32>(wcslen(guide)));
		return;
	}

	// 삽화 슬라이드쇼
	// 마지막 슬라이드의 페이드아웃이 끝나면 Update()가 _slideIndex를 배열 범위 밖(3)으로 올리고
	// ChangeScene을 예약하는데, 실제 씬 전환은 다음 프레임에야 일어나므로 이번 프레임 Render()가
	// 한 번 더 호출된다. 범위를 벗어난 인덱스로 배열을 읽지 않도록 clamp한다.
	const wchar_t* slideKeys[] = { L"EndSlide1", L"EndSlide2", L"EndSlide3" };
	int32 renderSlideIndex = (_slideIndex < 2) ? _slideIndex : 2;
	Texture* slide = ResourceManager::GetInstance().GetTexture(slideKeys[renderSlideIndex]);
	if (slide)
	{
		slide->RenderScreen(hdc, Vector(GWindowSizeX / 2.0f, GWinSizeY / 2.0f), Vector(0, 0), Vector((float)GWindowSizeX, (float)GWinSizeY));
	}

	// 마지막 슬라이드에서 페이드아웃 시작 전까지 Z 입력 안내를 보여준다.
	if (_slideIndex == 2 && !_fadingOut)
	{
		HFONT prevFont = _font ? (HFONT)SelectObject(hdc, _font) : nullptr;
		COLORREF prevColor = SetTextColor(hdc, RGB(255, 255, 255));
		int32 prevBkMode = SetBkMode(hdc, TRANSPARENT);

		const wchar_t* guide = L"Press Z";
		::TextOut(hdc, GWindowSizeX / 2 - 40, GWinSizeY - 60, guide, static_cast<int32>(wcslen(guide)));

		SetTextColor(hdc, prevColor);
		SetBkMode(hdc, prevBkMode);
		if (prevFont)
		{
			SelectObject(hdc, prevFont);
		}
	}

	// 검은 화면으로 페이드아웃
	if (_fadingOut)
	{
		float t = _slideTimer / SLIDE_FADE_DURATION;
		if (t > 1.f) t = 1.f;
		if (t < 0.f) t = 0.f;
		BYTE alpha = (BYTE)(255.f * t);

		HDC memDC = CreateCompatibleDC(hdc);
		HBITMAP memBmp = CreateCompatibleBitmap(hdc, 1, 1);
		HBITMAP prevBmp = (HBITMAP)SelectObject(memDC, memBmp);
		SetPixel(memDC, 0, 0, RGB(0, 0, 0));

		BLENDFUNCTION bf{};
		bf.BlendOp = AC_SRC_OVER;
		bf.BlendFlags = 0;
		bf.SourceConstantAlpha = alpha;
		bf.AlphaFormat = 0;
		AlphaBlend(hdc, 0, 0, GWindowSizeX, GWinSizeY, memDC, 0, 0, 1, 1, bf);

		SelectObject(memDC, prevBmp);
		DeleteObject(memBmp);
		DeleteDC(memDC);
	}
}
