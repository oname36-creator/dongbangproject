#include "pch.h"
#include "UIManager.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "Game.h"
#include "GameScene.h"
#include "Player.h"
#include "Boss.h"

void UIManager::Init()
{
	_hpTexture = ResourceManager::GetInstance().GetTexture(L"PlayerHP");

	ResourceManager::GetInstance().LoadTexture(L"UISidebarBG", L"UISidebarBG.bmp", -1);
	Texture* sidebarBg = ResourceManager::GetInstance().GetTexture(L"UISidebarBG");
	if (sidebarBg)
	{
		sidebarBg->SetApplyCenter(false);
	}

	fs::path fontPath = ResourceManager::GetInstance().GetResourcePath() / L"Fonts/Griun_Fromsol-Rg.ttf";
	AddFontResourceExW(fontPath.c_str(), FR_PRIVATE, 0);
	_logoFont = CreateFont(-40, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"Griun Fromsol");
	_statFont = CreateFont(-20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"Griun Fromsol");
}

void UIManager::Update(float deltaTime)
{
}

void UIManager::Render(HDC hdc)
{


	GameScene* scene = Game::GetInstance().GetScene();
	if (scene == nullptr)
		return;

	Player* player = scene->GetPlayer();
	if (player == nullptr)
		return;

	int32 lives = player->GetLives();
	if (lives < 0)
		lives = 0;

	// 사이드바 영역 (플레이필드 오른쪽, GWinSizeX ~ GWindowSizeX)
	int32 sidebarX = GWinSizeX + 20;

	// 사이드바 배경 타일
	Texture* sidebarBg = ResourceManager::GetInstance().GetTexture(L"UISidebarBG");
	if (sidebarBg)
	{
		sidebarBg->Render(hdc, Vector((float)GWinSizeX, 0.f));
	}

	COLORREF prevColor = SetTextColor(hdc, RGB(255, 255, 255));
	int32 prevBkMode = SetBkMode(hdc, TRANSPARENT);
	HFONT prevStatFont = _statFont ? (HFONT)SelectObject(hdc, _statFont) : nullptr;

	wstring scoreStr = std::format(L"점수 : {0}", scene->GetScore());
	::TextOut(hdc, sidebarX, 40, scoreStr.c_str(), static_cast<int32>(scoreStr.size()));

	wstring lifeStr = std::format(L"잔기 : {0}", lives);
	::TextOut(hdc, sidebarX, 90, lifeStr.c_str(), static_cast<int32>(lifeStr.size()));

	wstring bombStr = std::format(L"폭탄 : {0}", player->GetBoom());
	::TextOut(hdc, sidebarX, 115, bombStr.c_str(), static_cast<int32>(bombStr.size()));

	SetTextColor(hdc, RGB(230, 60, 60));
	wstring powerStr = std::format(L"파워 : {0}", player->GetPowerStack());
	::TextOut(hdc, sidebarX, 165, powerStr.c_str(), static_cast<int32>(powerStr.size()));

	SetTextColor(hdc, prevColor);

	// Power와 로고 사이 빈 공간에 조작키 안내를 넣는다.
	SetTextColor(hdc, RGB(200, 200, 200));
	const wchar_t* keyGuide[] = { L"방향키 : 화살표", L"공격 : Z", L"폭탄 : X", L"저속이동 : Shift" };
	int32 keyGuideY = 280;
	for (const wchar_t* line : keyGuide)
	{
		::TextOut(hdc, sidebarX, keyGuideY, line, static_cast<int32>(wcslen(line)));
		keyGuideY += 32;
	}
	SetTextColor(hdc, prevColor);

	if (prevStatFont)
	{
		SelectObject(hdc, prevStatFont);
	}

	// 하단 로고 자리: "동방모작" 2x2 배열
	if (_logoFont)
	{
		HFONT prevFont = (HFONT)SelectObject(hdc, _logoFont);
		SetTextColor(hdc, RGB(220, 220, 220));

		const wchar_t* logoChars = L"동방모작";
		int32 logoX = GWinSizeX + 55;
		int32 logoY = GWinSizeY - 220;
		int32 cellSize = 60;
		for (int32 i = 0; i < 4; ++i)
		{
			int32 col = i % 2;
			int32 row = i / 2;
			::TextOut(hdc, logoX + col * cellSize, logoY + row * cellSize, logoChars + i, 1);
		}

		SetTextColor(hdc, prevColor);
		SelectObject(hdc, prevFont);
	}

	SetBkMode(hdc, prevBkMode);

	Boss* boss = scene->GetBoss();
if (boss != nullptr)
{
    int32 barX = 50, barY = 20, barWidth = GWinSizeX - 100, barHeight = 15;

    // 배경 바 (회색)
    RECT bgRect = { barX, barY, barX + barWidth, barY + barHeight };
    HBRUSH grayBrush = CreateSolidBrush(RGB(80, 80, 80));
    FillRect(hdc, &bgRect, grayBrush);
    DeleteObject(grayBrush);

    // 현재 hp 비율만큼 채운 바 (빨강)
    float ratio = (float)boss->GetHp() / (float)boss->GetMaxHp();
    RECT hpRect = { barX, barY, barX + (int32)(barWidth * ratio), barY + barHeight };
    HBRUSH redBrush = CreateSolidBrush(RGB(200, 30, 30));
    FillRect(hdc, &hpRect, redBrush);
    DeleteObject(redBrush);

    HFONT prevPhaseFont = _statFont ? (HFONT)SelectObject(hdc, _statFont) : nullptr;
    wstring phaseStr = std::format(L"페이즈 {0}/{1}", boss->GetCurPhaseIndex() + 1, boss->GetPhaseCount());
    ::TextOut(hdc, barX, barY - 20, phaseStr.c_str(), (int32)phaseStr.size());
    if (prevPhaseFont)
    {
        SelectObject(hdc, prevPhaseFont);
    }
}
}
