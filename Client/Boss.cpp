#include "pch.h"
#include "Boss.h"
#include "Bullet.h"
#include "Game.h"
#include "GameScene.h"
#include "Player.h"
#include "TimeManager.h"
#include "colliderCircle.h"

void Boss::Init(Vector pos, wstring key)
{
	SetPos(pos);
	loadTexture(key);
_shootTimerId = TimeManager::GetInstance().AddTimer([this]() {shootBullet();}, 1.0f, true);
	_hp = 100;
	_phases = {{BossPatternType::Fan, 50},{BossPatternType::Circle, 0}};
	_moveTargetPos = Vector(GWinSizeX*0.5f, 150.f);

	_moveTimerId = TimeManager::GetInstance().AddTimer([this]() 
	{
		_moveTargetPos.x = GWinSizeX*0.25f + (rand()%(int)(GWinSizeX*0.5f));
		_moveTargetPos.y = 150.f;
	}, 3.0f, true);

}

void Boss::Destroy()
{
	Super::Destroy();
	TimeManager::GetInstance().Remove(_shootTimerId);
	TimeManager::GetInstance().Remove(_moveTimerId);
}

void Boss::Update(float deltaTime)
{
	Super::Update(deltaTime);

	Vector dir = _moveTargetPos - GetPos();
	dir.Normalize();
	Vector pos = GetPos();
	pos += dir * (_moveSpeed * deltaTime);
	SetPos(pos);
	
}

void Boss::Render(HDC hdc)
{
	Super::Render(hdc);
}

void Boss::OnEnter(Actor* other) // other : Bullet
{
	if (other->GetRenderLayer() == RenderLayer::Bullet)
	{
		Bullet* bullet = static_cast<Bullet*>(other);
		if (bullet && bullet->GetBulletType() == BulletType::Player)
		{
			_hp -= 1;

			// 마지막 페이즈가 아니고, 현재 페이즈의 임계값 밑으로 떨어졌다면 다음 페이즈로 전환
			if (_curPhaseIndex < (int32)_phases.size() - 1 &&_hp <= _phases[_curPhaseIndex].hpThreshold)
			{
				transitionToNextPhase();
			}

			if (_hp <= 0)
			{
				Destroy();
				Game::GetInstance().GetScene()->CreateEffect(GetPos());
				Game::GetInstance().GetScene()->AddScore(10000);
			}
		}
	}
}

void Boss::transitionToNextPhase()
{
	// 기존 탄 삭제
	Game::GetInstance().GetScene()->ClearEnemyBullets();

	// 짧은 연출 (일단은 이펙트로 대체 - Day4는 뼈대만 있으면 됨)
	Game::GetInstance().GetScene()->CreateEffect(GetPos());

	_curPhaseIndex++;
}

void Boss::shootBullet()
{
	switch(_phases[_curPhaseIndex].pattern)
	{
		case BossPatternType::Fan : 
			Game::GetInstance().GetScene()->FireFan(GetPos(), BulletType::Enemy, Vector(0,1), 60.f, 5, 300.f);
			break;
		case BossPatternType::Circle : 
			Game::GetInstance().GetScene()->FireCircle(GetPos(), BulletType::Enemy, 12, 300.f);
			break;
		
	}
}
