#include "pch.h"
#include "EndingScene.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "AudioManager.h"

namespace
{
	constexpr float SLIDE_HOLD_DURATION = 2.0f;	// end01/end02가 페이드아웃 전까지 떠있는 시간
	constexpr float SLIDE_FADE_DURATION = 0.8f;	// 검은 화면으로 페이드아웃되는 시간
	constexpr float EXTRA_CREDIT_DURATION = 3.0f;	// 엑스트라 클리어 문구가 떠있는 시간
}

void EndingScene::Init()
{
	// 일반/엑스트라 엔딩 공통으로 재생.
	AudioManager::GetInstance().PlayBGM(L"EndingBGM");

	fs::path fontPath = ResourceManager::GetInstance().GetResourcePath() / L"Fonts/Griun_Fromsol-Rg.ttf";
	AddFontResourceExW(fontPath.c_str(), FR_PRIVATE, 0);
	_font = CreateFont(-28, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"Griun Fromsol");

	if (_isExtra)
	{
		// 점수/삽화 없이 문구만 보여주므로 다른 텍스처는 로드할 필요가 없다.
		_creditFont = CreateFont(-48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
			HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE, L"Griun Fromsol");
		_smallCreditFont = CreateFont(-10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE, L"Griun Fromsol");
		_phase = EndingPhase::ExtraCredit;
		return;
	}

	ResourceManager::GetInstance().LoadTexture(L"StageResultBG", L"StageResultBG.bmp", -1);
	ResourceManager::GetInstance().LoadTexture(L"EndSlide1", L"end01.bmp", -1);
	ResourceManager::GetInstance().LoadTexture(L"EndSlide2", L"end02.bmp", -1);
	ResourceManager::GetInstance().LoadTexture(L"EndSlide3", L"end06.bmp", -1);
}

void EndingScene::Cleanup()
{
	if (_font)
	{
		DeleteObject(_font);
		_font = nullptr;
	}
	if (_creditFont)
	{
		DeleteObject(_creditFont);
		_creditFont = nullptr;
	}
	if (_smallCreditFont)
	{
		DeleteObject(_smallCreditFont);
		_smallCreditFont = nullptr;
	}

	AudioManager::GetInstance().StopBGM();

	fs::path fontPath = ResourceManager::GetInstance().GetResourcePath() / L"Fonts/Griun_Fromsol-Rg.ttf";
	RemoveFontResourceExW(fontPath.c_str(), FR_PRIVATE, 0);
}

void EndingScene::Update(float deltaTime)
{
	if (_phase == EndingPhase::ExtraCredit)
	{
		_extraCreditTimer += deltaTime;
		if (_extraCreditTimer >= EXTRA_CREDIT_DURATION)
		{
			SceneManager::GetInstance().ChangeScene(new TitleScene());
		}
		return;
	}

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

	if (_phase == EndingPhase::ExtraCredit)
	{
		HFONT prevFont = _creditFont ? (HFONT)SelectObject(hdc, _creditFont) : nullptr;
		COLORREF prevColor = SetTextColor(hdc, RGB(255, 255, 255));
		int32 prevBkMode = SetBkMode(hdc, TRANSPARENT);

		const wchar_t* line1 = L"2026년 9월 10일 동방홍마향remake 많관부~";
		const wchar_t* line2 = L"제작자 9.8";

		RECT line1Rect{ 0, GWinSizeY / 2 - 60, GWindowSizeX, GWinSizeY / 2 };
		RECT line2Rect{ 0, GWinSizeY / 2 + 10, GWindowSizeX, GWinSizeY / 2 + 70 };
		DrawText(hdc, line1, -1, &line1Rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);
		DrawText(hdc, line2, -1, &line2Rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);

		if (_smallCreditFont)
		{
			SelectObject(hdc, _smallCreditFont);

			const wchar_t* line3 = L"기획 아이디어 : 김민성";
			const wchar_t* line4 = L"회전함수 제공 : 이승호";

			RECT line3Rect{ 0, GWinSizeY / 2 + 90, GWindowSizeX, GWinSizeY / 2 + 105 };
			RECT line4Rect{ 0, GWinSizeY / 2 + 105, GWindowSizeX, GWinSizeY / 2 + 120 };
			DrawText(hdc, line3, -1, &line3Rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);
			DrawText(hdc, line4, -1, &line4Rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);
		}

		SetTextColor(hdc, prevColor);
		SetBkMode(hdc, prevBkMode);
		if (prevFont)
		{
			SelectObject(hdc, prevFont);
		}
		return;
	}

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
