#include "pch.h"
#include "UIManager.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "Game.h"
#include "GameScene.h"
#include "Player.h"

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
}
