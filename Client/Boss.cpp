#include "pch.h"
#include "Boss.h"
#include "Bullet.h"
#include "Game.h"
#include "GameScene.h"
#include "Player.h"
#include "TimeManager.h"
#include "colliderCircle.h"

namespace
{
	// 패턴별 재발동 간격. 예고 판정은 지연(1.8초) + 여유를 둬서 경고가 겹쳐 쌓이지 않게 한다.
	float GetShootInterval(BossPatternType pattern)
	{
		if (pattern == BossPatternType::Telegraph)
			return 2.5f;

		return 1.0f;
	}
}

void Boss::Init(Vector pos, wstring key, vector<BossPhase> phases, int32 maxHp)
{
	SetPos(pos);
	loadTexture(key);
	_maxHp = maxHp;
	_hp = maxHp;
	_curPhaseIndex = 0;
	_phases = phases;
	_shootTimerId = TimeManager::GetInstance().AddTimer([this]() {shootBullet();}, GetShootInterval(_phases[_curPhaseIndex].pattern), true);
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
	TimeManager::GetInstance().Remove(_spiralShootTimerId);
	TimeManager::GetInstance().Remove(_telegraphTimerId);
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
			bullet->Destroy();

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

	// 페이즈가 바뀌면 재발동 간격도 새 패턴에 맞게 다시 건다.
	TimeManager::GetInstance().Remove(_shootTimerId);
	_shootTimerId = TimeManager::GetInstance().AddTimer([this]() { shootBullet(); }, GetShootInterval(_phases[_curPhaseIndex].pattern), true);

	if(_phases[_curPhaseIndex].pattern == BossPatternType::Spiral)
	{
		_spiralShootTimerId = TimeManager::GetInstance().AddTimer([this]() { shootSpiralBullet();}, 0.1f, true);
	}
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
		case BossPatternType::Homing :
		{
			Vector dir(0, 1);
			Player* player = Game::GetInstance().GetScene()->GetPlayer();
			if (player != nullptr)
			{
				dir = player->GetPos() - GetPos();
				dir.Normalize();
			}
			Game::GetInstance().GetScene()->FireHoming(GetPos(), BulletType::Enemy, dir, 250.f, 150.f);
			break;
		}
		case BossPatternType::Telegraph :
			shootTelegraphBullet();
			break;
	}
}

void Boss::shootSpiralBullet()
{
Game::GetInstance().GetScene()->FireSpiral(GetPos(), BulletType::Enemy, 2, 300.f, _spiralAngle, 10.f );
}

void Boss::shootTelegraphBullet()
{
	// 예고 판정: 경고 지점을 정하고(플레이어의 현재 위치), 그 자리에 경고 표시 후
	// 일정 시간 뒤 그 위치를 중심으로 원형탄을 터뜨린다. 플레이어는 경고가 뜬 동안 벗어나야 한다.
	Player* player = Game::GetInstance().GetScene()->GetPlayer();
	if (player == nullptr)
		return;

	Vector warnPos = player->GetPos();
	Game::GetInstance().GetScene()->CreateEffect(warnPos);

	_telegraphTimerId = TimeManager::GetInstance().AddTimer([warnPos]()
	{
		Game::GetInstance().GetScene()->FireCircle(warnPos, BulletType::Enemy, 10, 250.f);
	}, 1.8f, false);
}
