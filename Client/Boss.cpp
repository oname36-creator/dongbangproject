#include "pch.h"
#include "Boss.h"
#include "Bullet.h"
#include "Game.h"
#include "GameScene.h"
#include "Player.h"
#include "TimeManager.h"
#include "colliderCircle.h"
#include "SpriteRenderer.h"
#include "ResourceManager.h"

namespace
{
	// Spiral은 타임라인의 "정해진 시각에 한 번 발사" 모델이 아니라, 페이즈 내내
	// 0.1초 간격으로 계속 도는 연속 발사(_spiralShootTimerId)로 처리된다.
	// 그래서 페이즈 진입/전환 시 이 페이즈의 타임라인에 Spiral이 있는지만 확인하면 된다.
	bool ContainsSpiral(const vector<TimelineStep>& timeline)
	{
		for (const TimelineStep& step : timeline)
		{
			if (step.pattern == BossPatternType::Spiral)
				return true;
		}
		return false;
	}
}

void Boss::Init(Vector pos, wstring key, vector<BossPhase> phases, int32 maxHp,
				 float bulletCountMul, float bulletSpeedMul,
				 int32 spiralArmCount, float spiralRotationSpeed, float spiralInterval,
				 int32 randomShotCount, float randomAccel, bool randomAlternateAccel,
				 float fanAngleSpread, int32 fanShotCount,
				 int32 randomDelayedShotCount, float randomDelayedSpeed,
				 float randomDelayedPreStop, float randomDelayedDelay)
{
	SetPos(pos);

	_baseKey = key;
	SpriteAnimRenderer* renderer = GetComponent<SpriteAnimRenderer>();
	if (renderer == nullptr)
	{
		renderer = AddComponent<SpriteAnimRenderer>();
	}
	renderer->SetLoop(true);
	_animRenderer = renderer;
	_animState = BossAnimState::Idle;
	_animRenderer->Init(_baseKey + L"Idle");

	ColliderCircle* collider = GetComponent<ColliderCircle>();
	if (collider == nullptr)
	{
		collider = AddComponent<ColliderCircle>();
	}
	collider->Init(this, renderer->GetSizeX() * 0.7f);
	_collider = collider;

	_maxHp = maxHp;
	_bulletCountMul = bulletCountMul;
	_bulletSpeedMul = bulletSpeedMul;
	_spiralArmCount = spiralArmCount;
	_spiralRotationSpeed = spiralRotationSpeed;
	_spiralInterval = spiralInterval;
	_randomShotCount = randomShotCount;
	_randomAccel = randomAccel;
	_randomAlternateAccel = randomAlternateAccel;
	_fanAngleSpread = fanAngleSpread;
	_fanShotCount = fanShotCount;
	_randomDelayedShotCount = randomDelayedShotCount;
	_randomDelayedSpeed = randomDelayedSpeed;
	_randomDelayedPreStop = randomDelayedPreStop;
	_randomDelayedDelay = randomDelayedDelay;
	_hp = maxHp;
	_curPhaseIndex = 0;
	_phases = phases;
	_phaseElapsedTime = 0.f;
	_timelineIndex = 0;

	_moveTargetPos = Vector(GWinSizeX*0.5f, 150.f);

	_moveTimerId = TimeManager::GetInstance().AddTimer([this]()
	{
		_moveTargetPos.x = GWinSizeX*0.25f + (rand()%(int)(GWinSizeX*0.5f));
		_moveTargetPos.y = 150.f;
	}, 3.0f, true);

	if (ContainsSpiral(_phases[_curPhaseIndex].timeline))
	{
		_spiralShootTimerId = TimeManager::GetInstance().AddTimer([this]() { shootSpiralBullet(); }, _spiralInterval, true);
	}
}

void Boss::Destroy()
{
	
	Super::Destroy();

	_isDead = true;
	TimeManager::GetInstance().Remove(_moveTimerId);
	TimeManager::GetInstance().Remove(_spiralShootTimerId);
	TimeManager::GetInstance().Remove(_telegraphTimerId);
	TimeManager::GetInstance().Remove(_burstShootTimerId);
}

void Boss::Update(float deltaTime)
{
	Super::Update(deltaTime);

	Vector toTarget = _moveTargetPos - GetPos();
	float distToTarget = toTarget.Length();
	Vector dir = toTarget;
	dir.Normalize();
	Vector pos = GetPos();
	pos += dir * (_moveSpeed * deltaTime);
	SetPos(pos);

	if (_attackPoseTimer > 0.f)
	{
		_attackPoseTimer -= deltaTime;
		setAnimState(BossAnimState::Attack);
	}
	else if (distToTarget > 5.0f)
	{
		setAnimState(BossAnimState::Move);
	}
	else
	{
		setAnimState(BossAnimState::Idle);
	}

	_phaseElapsedTime += deltaTime;
	const vector<TimelineStep>& timeline = _phases[_curPhaseIndex].timeline;
	while (_timelineIndex < (int32)timeline.size() && timeline[_timelineIndex].time <= _phaseElapsedTime)
	{
		// Spiral은 연속 타이머(_spiralShootTimerId)가 따로 쏘고 있으므로 여기서는 건너뛴다.
		if (timeline[_timelineIndex].pattern != BossPatternType::Spiral)
		{
			shootBullet(timeline[_timelineIndex].pattern);
		}
		_timelineIndex++;
	}
	if (_phaseElapsedTime >= _phases[_curPhaseIndex].cycleLength)
	{
		_phaseElapsedTime = 0.f;
		_timelineIndex = 0;   // 사이클 리셋 -> 처음부터 반복
	}

}

void Boss::Render(HDC hdc)
{
	Super::Render(hdc);
}

void Boss::setAnimState(BossAnimState state)
{
	if (_animState == state || _animRenderer == nullptr)
		return;

	_animState = state;

	switch (state)
	{
		case BossAnimState::Idle :
			_animRenderer->Init(_baseKey + L"Idle");
			_animRenderer->SetLoop(true);
			break;
		case BossAnimState::Move :
			// 한 번만 재생하고 마지막 프레임에서 멈춘 채로 이동을 계속한다.
			_animRenderer->Init(_baseKey + L"Move");
			_animRenderer->SetLoop(false);
			break;
		case BossAnimState::Attack :
		{
			// Move와 마찬가지로 한 번만 재생하고 마지막 프레임에서 멈춘다.
			// 공격 전용 텍스처가 없는 보스(예: Boss2)는 Idle로 대체한다.
			wstring attackKey = _baseKey + L"Attack";
			if (ResourceManager::GetInstance().GetTexture(attackKey) == nullptr)
			{
				attackKey = _baseKey + L"Idle";
			}
			_animRenderer->Init(attackKey);
			_animRenderer->SetLoop(false);
			break;
		}
	}
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

	// 새 페이즈의 타임라인을 처음부터 재생하도록 리셋
	_phaseElapsedTime = 0.f;
	_timelineIndex = 0;

	// 이전 페이즈의 연속 스파이럴 타이머는 정리하고, 새 페이즈에 Spiral이 있으면 다시 건다.
	TimeManager::GetInstance().Remove(_spiralShootTimerId);
	if (ContainsSpiral(_phases[_curPhaseIndex].timeline))
	{
		_spiralShootTimerId = TimeManager::GetInstance().AddTimer([this]() { shootSpiralBullet();}, _spiralInterval, true);
	}
}

void Boss::shootBullet(BossPatternType pattern)
{
	_attackPoseTimer = 0.3f;

	switch(pattern)
	{
		case BossPatternType::Fan :
			Game::GetInstance().GetScene()->FireFan(GetPos(), BulletType::Enemy, Vector(0,1), _fanAngleSpread,
				(int32)(_fanShotCount * _bulletCountMul), 300.f * _bulletSpeedMul);
			break;
		case BossPatternType::Circle :
			Game::GetInstance().GetScene()->FireCircle(GetPos(), BulletType::Enemy,
				(int32)(12 * _bulletCountMul), 300.f * _bulletSpeedMul);
			break;
		case BossPatternType::AimedBurst :
		{
			Vector dir(0, 1);
			Player* player = Game::GetInstance().GetScene()->GetPlayer();
			if (player != nullptr)
			{
				dir = player->GetPos() - GetPos();
				dir.Normalize();
			}
			_burstDir = dir;
			_burstShotsRemaining = (int32)(8 * _bulletCountMul);
			_burstTotalShots = _burstShotsRemaining;

			// 이전 버스트가 아직 안 끝났으면 정리하고 새로 시작 (중복 타이머 방지)
			TimeManager::GetInstance().Remove(_burstShootTimerId);
			_burstShootTimerId = TimeManager::GetInstance().AddTimer([this]() { shootAimedBurst(); }, 0.04f, true);
			break;
		}
		case BossPatternType::Telegraph :
			shootTelegraphBullet();
			break;
		case BossPatternType::Grid :
			// 화면 폭(480px)에 걸쳐 6열 x 3행 격자, 아래로 스크롤.
			// origin.x를 보스 위치가 아니라 화면 왼쪽 기준(40px)으로 고정해야 화면 전체를 덮는 격자가 된다.
			Game::GetInstance().GetScene()->FireGrid(Vector(40.f, GetPos().y), BulletType::Enemy, Vector(0, 1), 3, 6, 80.f, 50.f, 220.f);
			break;
		case BossPatternType::Spiral :
			// Update()에서 이미 걸러내고 연속 타이머(_spiralShootTimerId)로 처리하므로 여기선 아무것도 안 함.
			break;
		case BossPatternType::Cross :
			Game::GetInstance().GetScene()->FireCross(GetPos().y, BulletType::Enemy, 300.f);
			break;
		case BossPatternType::Random :
		{
			_randomAccelToggle = !_randomAccelToggle;
			float accel = _randomAlternateAccel ? (_randomAccelToggle ? -10.f : _randomAccel) : _randomAccel;
			Game::GetInstance().GetScene()->FireRandom(GetPos(), BulletType::Enemy,
				(int32)(_randomShotCount * _bulletCountMul), 250.f * _bulletSpeedMul, accel);
			break;
		}
		case BossPatternType::CircleDelayedAimed :
			// 2초 날아가다 1초 정지 후 플레이어 조준으로 전환
			Game::GetInstance().GetScene()->FireCircle(GetPos(), BulletType::Enemy,
				(int32)(9 * _bulletCountMul), 300.f * _bulletSpeedMul, 2.0f, 1.0f, BulletRedirectMode::Aimed);
			break;
		case BossPatternType::CircleDelayedRandom :
			// 2.5초 날아가다 2초 정지 후 무작위 방향으로 전환
			Game::GetInstance().GetScene()->FireCircle(GetPos(), BulletType::Enemy,
				(int32)(9 * _bulletCountMul), 300.f * _bulletSpeedMul, 2.5f, 2.0f, BulletRedirectMode::Random);
			break;
		case BossPatternType::RandomDelayedRandom :
			// 무작위 난사 + 잠시 날아가다 정지 후 다시 무작위 방향으로 전환
			Game::GetInstance().GetScene()->FireRandom(GetPos(), BulletType::Enemy,
				(int32)(_randomDelayedShotCount * _bulletCountMul), _randomDelayedSpeed * _bulletSpeedMul, 0.f,
				_randomDelayedPreStop, _randomDelayedDelay, BulletRedirectMode::Random);
			break;
	}
}

void Boss::shootSpiralBullet()
{
	if (_isDead)
		return;

	GameScene* scene = Game::GetInstance().GetScene();
	if (scene == nullptr)
		return;

	_attackPoseTimer = 0.3f;
	scene->FireSpiral(GetPos(), BulletType::Enemy, (int32)(_spiralArmCount * _bulletCountMul), 300.f * _bulletSpeedMul, _spiralAngle, _spiralRotationSpeed);
}

void Boss::shootAimedBurst()
{
	if (_isDead)
		return;

	GameScene* scene = Game::GetInstance().GetScene();
	if (scene == nullptr)
		return;

	// 고정해둔 방향으로 한 발씩 연사. 총알들이 시간차를 두고 같은 경로를 따라가면서
	// 실시간으로 이어진 줄처럼 보인다.
	// 첫 발은 느리게, 뒤로 갈수록 점점 빨라져서 뒤에 쏜 총알이 앞선 총알을 따라잡는 느낌을 낸다.
	_attackPoseTimer = 0.3f;
	int32 shotIndex = _burstTotalShots - _burstShotsRemaining;
	float t = (_burstTotalShots > 1) ? (float)shotIndex / (_burstTotalShots - 1) : 0.f;
	constexpr float minSpeed = 150.f;
	constexpr float maxSpeed = 450.f;
	float speed = (minSpeed + (maxSpeed - minSpeed) * t) * _bulletSpeedMul;
	scene->FireStraight(GetPos(), BulletType::Enemy, _burstDir, speed);

	_burstShotsRemaining--;
	if (_burstShotsRemaining <= 0)
	{
		TimeManager::GetInstance().Remove(_burstShootTimerId);
	}
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

	_telegraphTimerId = TimeManager::GetInstance().AddTimer([warnPos, countMul = _bulletCountMul, speedMul = _bulletSpeedMul]()
	{
		// this를 캡처하지 않는다: Boss는 풀이 아니라 new/delete로 관리되므로,
		// 이 타이머가 delete된 Boss를 가리키는 채로 남아있을 수 있다 (use-after-free 방지).
		// 배율은 값으로 미리 복사해서 캡처한다.
		GameScene* scene = Game::GetInstance().GetScene();
		if (scene == nullptr)
			return;

		scene->FireCircle(warnPos, BulletType::Enemy, (int32)(10 * countMul), 250.f * speedMul);
	}, 1.8f, false);
}


