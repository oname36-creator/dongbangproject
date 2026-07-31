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

// TODO(1주차 Day6~7): HUD를 기획서 9장 사양으로 교체할 것
//  현재: HP를 10으로 나눠 하트를 그린다 (HP 100 체계 전용이라 잔기 체계로 바꾸면 깨진다)
//  목표: 좌측 상단 = 잔기 / 폭탄 / 파워단계,  우측 상단 = 점수 / 하이스코어
//  할 일: Player의 _life,_bomb 와 GameScene의 _score 를 읽어서 표시한다.
//  힌트: 이미지부터 만들지 마라. Game::Render()가 FPS를 찍는 것처럼
//        ::TextOut + std::format 으로 시작하면 10분이면 된다. 폴리싱은 4주차 몫이다.
//  주의: Player에서 GetHp()가 사라지면 이 파일이 제일 먼저 컴파일 에러를 낸다.
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
	
	wstring lifeStr = std::format(L"life: {0} Bomb: {1}", lives, player->GetBoom());
	::TextOut(hdc, 10, 40, lifeStr.c_str(), static_cast<int32>(lifeStr.size()));

	wstring scoreStr = std::format(L"Score : {0}", scene->GetScore());
	::TextOut(hdc, GWinSizeX -150, 40, scoreStr.c_str(), static_cast<int32>(scoreStr.size()));

	// TODO(2주차 Day5): 보스 체력바 + 이름/패턴명 표시
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

	//  1. GameScene에 GetBoss() 같은 getter가 필요하다 (GetPlayer()와 동일한 패턴).
	//  2. Boss에도 GetHp()/GetMaxHp() 같은 getter가 필요하다 (지금 _hp는 private, GameScene도 못 읽는다).
	//  3. scene->GetBoss()가 nullptr이 아닐 때만 그려라(보스 페이즈가 아니면 아무것도 안 그림).
	//  4. 이미지 새로 만들지 말고 Rectangle(hdc, ...) 두 겹으로 시작해라:
	//     배경 바(회색) 위에 현재 hp 비율만큼 채운 바(빨강) - 기존 HUD 원칙(TextOut)과 같다.
	//  5. 페이즈 이름도 같이 찍고 싶으면 Boss에 현재 페이즈 인덱스를 읽는 getter를 추가해서
	//     "Phase 1/3" 처럼 TextOut으로 찍으면 충분하다.
}
