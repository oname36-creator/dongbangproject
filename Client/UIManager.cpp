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

	wstring scoreStr = std::format(L"Score : {0}", scene->GetScore());
	::TextOut(hdc, sidebarX, 40, scoreStr.c_str(), static_cast<int32>(scoreStr.size()));

	wstring lifeStr = std::format(L"life: {0} Bomb: {1}", lives, player->GetBoom());
	::TextOut(hdc, sidebarX, 70, lifeStr.c_str(), static_cast<int32>(lifeStr.size()));

	wstring powerStr = std::format(L"Power: {0}", player->GetPowerStack());
	::TextOut(hdc, sidebarX, 100, powerStr.c_str(), static_cast<int32>(powerStr.size()));

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

    wstring phaseStr = std::format(L"Phase {0}/{1}", boss->GetCurPhaseIndex() + 1, boss->GetPhaseCount());
    ::TextOut(hdc, barX, barY - 20, phaseStr.c_str(), (int32)phaseStr.size());
}
}
