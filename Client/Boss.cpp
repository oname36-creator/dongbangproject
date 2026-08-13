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
#include "Texture.h"
#include <random>
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

	// Cross도 Spiral과 같은 이유로, 타임라인에 있는지만 확인해서 연속 타이머를 건다.
	bool ContainsCross(const vector<TimelineStep>& timeline)
	{
		for (const TimelineStep& step : timeline)
		{
			if (step.pattern == BossPatternType::Cross)
				return true;
		}
		return false;
	}

	// ConvergingBurst도 Spiral/Cross와 같은 이유로, 타임라인에 있는지만 확인해서 연속 타이머를 건다.
	bool ContainsConvergingBurst(const vector<TimelineStep>& timeline)
	{
		for (const TimelineStep& step : timeline)
		{
			if (step.pattern == BossPatternType::ConvergingBurst)
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
				 float randomDelayedPreStop, float randomDelayedDelay,
				int32 spreadShotCount, float spreadAngle,
			float spreadMinSpeed, float spreadMaxSpeed,
			int32 circleShotCount, wstring circleTextureKey,
			wstring burstTextureKey, float circleColliderSize,
			int32 decorPhaseIndex,
			wstring burstTextureKeyAlt, int32 burstTextureAltPhaseIndex, float burstColliderSizeAlt,
			wstring randomTextureKey, wstring spiralTextureKey, wstring fanTextureKey, wstring randomDelayedTextureKey,
			bool bulletsFaceDirection, float faceDirectionColliderSize,
			wstring circleDelayedAimedTextureKey, wstring circleDelayedRandomTextureKey,
			int32 fixedPosPhaseIndex, float circleRotationSpeed,
			float convergingSpeed, float convergingSpeedSpread, float convergingInterval,
			float convergingAngleSpread)
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
	_spreadShotCount = spreadShotCount;
	_spreadAngle = spreadAngle;
	_spreadMinSpeed = spreadMinSpeed;
	_spreadMaxSpeed = spreadMaxSpeed;
	_circleShotCount = circleShotCount;
	_circleTextureKey = circleTextureKey;
	_burstTextureKey = burstTextureKey;
	_circleColliderSize = circleColliderSize;
	_decorPhaseIndex = decorPhaseIndex;
	_burstTextureKeyAlt = burstTextureKeyAlt;
	_burstTextureAltPhaseIndex = burstTextureAltPhaseIndex;
	_burstColliderSizeAlt = burstColliderSizeAlt;
	_randomTextureKey = randomTextureKey;
	_spiralTextureKey = spiralTextureKey;
	_fanTextureKey = fanTextureKey;
	_randomDelayedTextureKey = randomDelayedTextureKey;
	_bulletsFaceDirection = bulletsFaceDirection;
	_faceDirectionColliderSize = faceDirectionColliderSize;
	_circleDelayedAimedTextureKey = circleDelayedAimedTextureKey;
	_circleDelayedRandomTextureKey = circleDelayedRandomTextureKey;
	_fixedPosPhaseIndex = fixedPosPhaseIndex;
	_circleRotationSpeed = circleRotationSpeed;
	_convergingSpeed = convergingSpeed;
	_convergingSpeedSpread = convergingSpeedSpread;
	_convergingInterval = convergingInterval;
	_convergingAngleSpread = convergingAngleSpread;
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
	if (ContainsCross(_phases[_curPhaseIndex].timeline))
	{
		_crossShootTimerId = TimeManager::GetInstance().AddTimer([this]() { shootCrossBullet(); }, 0.04f, true);
	}
	if (ContainsConvergingBurst(_phases[_curPhaseIndex].timeline))
	{
		_convergingBurstTimerId = TimeManager::GetInstance().AddTimer([this]() { shootConvergingBurst(); }, _convergingInterval, true);
	}
}

void Boss::Destroy()
{
	
	Super::Destroy();

	_isDead = true;
	TimeManager::GetInstance().Remove(_moveTimerId);
	TimeManager::GetInstance().Remove(_spiralShootTimerId);
	TimeManager::GetInstance().Remove(_crossShootTimerId);
	TimeManager::GetInstance().Remove(_convergingBurstTimerId);
	TimeManager::GetInstance().Remove(_telegraphTimerId);
	TimeManager::GetInstance().Remove(_burstShootTimerId);
	TimeManager::GetInstance().Remove(_spreadShootTimerId);
}

void Boss::Update(float deltaTime)
{
	Super::Update(deltaTime);

	// 이 페이즈 동안엔 랜덤 이동 타이머가 뭘 정해놨든 무시하고 매 프레임 중앙을 목표로 고정한다.
	if (_curPhaseIndex == _fixedPosPhaseIndex)
	{
		_moveTargetPos = Vector(GWinSizeX * 0.5f, 150.f);
	}

	Vector toTarget = _moveTargetPos - GetPos();
	float distToTarget = toTarget.Length();
	Vector dir = toTarget;
	dir.Normalize();
	Vector pos = GetPos();
	pos += dir * (_moveSpeed * deltaTime);
	SetPos(pos);

	// 공격 포즈 타이머는 항상 흘러가되, 실제로 이동 중이면 Move가 Attack보다 우선한다.
	if (_attackPoseTimer > 0.f)
	{
		_attackPoseTimer -= deltaTime;
	}

	if (distToTarget > 5.0f)
	{
		setAnimState(BossAnimState::Move);
	}
	else if (_attackPoseTimer > 0.f)
	{
		setAnimState(BossAnimState::Attack);
	}
	else
	{
		setAnimState(BossAnimState::Idle);
	}

	_phaseElapsedTime += deltaTime;
	const vector<TimelineStep>& timeline = _phases[_curPhaseIndex].timeline;
	while (_timelineIndex < (int32)timeline.size() && timeline[_timelineIndex].time <= _phaseElapsedTime)
	{
		// Spiral/Cross/ConvergingBurst는 연속 타이머가 따로 쏘고 있으므로 여기서는 건너뛴다.
		if (timeline[_timelineIndex].pattern != BossPatternType::Spiral &&
			timeline[_timelineIndex].pattern != BossPatternType::Cross &&
			timeline[_timelineIndex].pattern != BossPatternType::ConvergingBurst)
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
	// 지정된 페이즈에서만, 보스 스프라이트보다 먼저 그려서 뒤에 깔리게 한다.
	// 매 프레임 GetPos()를 그대로 쓰기 때문에 보스가 이동하면 같이 따라간다.
	if (_curPhaseIndex == _decorPhaseIndex)
	{
		Texture* magicCircle = ResourceManager::GetInstance().GetTexture(L"MagicCircle");
		if (magicCircle)
		{
			magicCircle->Render(hdc, GetPos());
		}
	}

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

	// 이전 페이즈의 연속 스파이럴/크로스 타이머는 정리하고, 새 페이즈에 있으면 다시 건다.
	TimeManager::GetInstance().Remove(_spiralShootTimerId);
	if (ContainsSpiral(_phases[_curPhaseIndex].timeline))
	{
		_spiralShootTimerId = TimeManager::GetInstance().AddTimer([this]() { shootSpiralBullet();}, _spiralInterval, true);
	}
	TimeManager::GetInstance().Remove(_crossShootTimerId);
	if (ContainsCross(_phases[_curPhaseIndex].timeline))
	{
		_crossShootTimerId = TimeManager::GetInstance().AddTimer([this]() { shootCrossBullet(); }, 0.04f, true);
	}
	TimeManager::GetInstance().Remove(_convergingBurstTimerId);
	if (ContainsConvergingBurst(_phases[_curPhaseIndex].timeline))
	{
		_convergingBurstTimerId = TimeManager::GetInstance().AddTimer([this]() { shootConvergingBurst(); }, _convergingInterval, true);
	}
}

void Boss::shootBullet(BossPatternType pattern)
{
	_attackPoseTimer = 0.3f;

	switch(pattern)
	{
		case BossPatternType::Fan :
			Game::GetInstance().GetScene()->FireFan(GetPos(), BulletType::Enemy, Vector(0,1), _fanAngleSpread,
				(int32)(_fanShotCount * _bulletCountMul), 300.f * _bulletSpeedMul,
				_fanTextureKey, _fanTextureKey.empty() ? -1.f : _faceDirectionColliderSize, _bulletsFaceDirection);
			break;
		case BossPatternType::Circle :
			Game::GetInstance().GetScene()->FireCircle(GetPos(), BulletType::Enemy,
				(int32)(_circleShotCount * _bulletCountMul), 300.f * _bulletSpeedMul,
				0.f, 0.f, BulletRedirectMode::None, _circleTextureKey, _circleColliderSize, _bulletsFaceDirection, _circleAngle);
			_circleAngle += _circleRotationSpeed;	// 다음 Circle은 이만큼 회전된 각도에서 시작 (0이면 매번 그대로)
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
			// 화면 폭(GWinSizeX) 전체에 걸쳐 6열 x 3행 격자, 아래로 스크롤.
			// origin.x/y를 보스 위치가 아니라 화면 기준 고정값으로 써야, 발동될 때마다
			// 줄이 어긋나지 않고 항상 같은 자리에서 격자가 시작된다.
			// 속도(200) x 사이클(0.5초) = 100 = 행간격(100) x 1 로 맞춰서, 새 웨이브가 이전 웨이브들의
			// 빈 줄 사이에 정확히 끼워지게 한다 (안 맞으면 웨이브끼리 어긋나 두 줄이 거의 붙어 보인다).
			// GridBulletRed도 30x30이라 기본 콜라이더(27)의 절반인 13.5로 줄임.
			Game::GetInstance().GetScene()->FireGrid(Vector(40.f, -30.f), BulletType::Enemy, Vector(0, 1), 3, 6, (GWinSizeX - 80.f) / 5.f, 100.f, 200.f, L"GridBulletRed", 13.5f);
			break;
		case BossPatternType::Spiral :
			// Update()에서 이미 걸러내고 연속 타이머(_spiralShootTimerId)로 처리하므로 여기선 아무것도 안 함.
			break;
		case BossPatternType::Cross :
			// Update()에서 이미 걸러내고 연속 타이머(_crossShootTimerId)로 처리하므로 여기선 아무것도 안 함.
			break;
		case BossPatternType::ConvergingBurst :
			// Update()에서 이미 걸러내고 연속 타이머(_convergingBurstTimerId)로 처리하므로 여기선 아무것도 안 함.
			break;
		case BossPatternType::Random :
		{
			_randomAccelToggle = !_randomAccelToggle;
			float accel = _randomAlternateAccel ? (_randomAccelToggle ? -10.f : _randomAccel) : _randomAccel;
			Game::GetInstance().GetScene()->FireRandom(GetPos(), BulletType::Enemy,
				(int32)(_randomShotCount * _bulletCountMul), 250.f * _bulletSpeedMul, accel,
				0.f, 0.f, BulletRedirectMode::None, _randomTextureKey, _randomTextureKey.empty() ? -1.f : _faceDirectionColliderSize, _bulletsFaceDirection);
			break;
		}
		case BossPatternType::CircleDelayedAimed :
			// 2초 날아가다 1초 정지 후 플레이어 조준으로 전환
			Game::GetInstance().GetScene()->FireCircle(GetPos(), BulletType::Enemy,
				(int32)(9 * _bulletCountMul), 300.f * _bulletSpeedMul, 2.0f, 1.0f, BulletRedirectMode::Aimed,
				_circleDelayedAimedTextureKey, _circleDelayedAimedTextureKey.empty() ? -1.f : _faceDirectionColliderSize, _bulletsFaceDirection);
			break;
		case BossPatternType::CircleDelayedRandom :
			// 2.5초 날아가다 2초 정지 후 무작위 방향으로 전환
			Game::GetInstance().GetScene()->FireCircle(GetPos(), BulletType::Enemy,
				(int32)(9 * _bulletCountMul), 300.f * _bulletSpeedMul, 2.5f, 2.0f, BulletRedirectMode::Random,
				_circleDelayedRandomTextureKey, _circleDelayedRandomTextureKey.empty() ? -1.f : _faceDirectionColliderSize, _bulletsFaceDirection);
			break;
		case BossPatternType::RandomDelayedRandom :
			// 무작위 난사 + 잠시 날아가다 정지 후 다시 무작위 방향으로 전환
			Game::GetInstance().GetScene()->FireRandom(GetPos(), BulletType::Enemy,
				(int32)(_randomDelayedShotCount * _bulletCountMul), _randomDelayedSpeed * _bulletSpeedMul, 0.f,
				_randomDelayedPreStop, _randomDelayedDelay, BulletRedirectMode::Random,
				_randomDelayedTextureKey, _randomDelayedTextureKey.empty() ? -1.f : _faceDirectionColliderSize, _bulletsFaceDirection);
			break;
		case BossPatternType::AimedSpread :
		{
			Vector dir(0,1);
			Player*player = Game::GetInstance().GetScene()->GetPlayer();
			if(player != nullptr)
			{
				dir = player->GetPos() - GetPos();
				dir.Normalize();
			}
			_spreadBaseDir = dir;
			_spreadShotsRemaining = (int32)(_spreadShotCount * _bulletCountMul);
			_spreadShootTimerId = TimeManager::GetInstance().AddTimer([this]() {shootSpreadBullet();}, 1.f/ 30.f, true);
			break;
		}
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
	scene->FireSpiral(GetPos(), BulletType::Enemy, (int32)(_spiralArmCount * _bulletCountMul), 300.f * _bulletSpeedMul, _spiralAngle, _spiralRotationSpeed,
		_spiralTextureKey, _spiralTextureKey.empty() ? -1.f : _faceDirectionColliderSize, _bulletsFaceDirection);
}

void Boss::shootCrossBullet()
{
	if (_isDead)
		return;

	GameScene* scene = Game::GetInstance().GetScene();
	if (scene == nullptr)
		return;

	_attackPoseTimer = 0.3f;

	// FireCross()와 동일한 좌/우 대각선 방향. 매 호출마다 양쪽 다 쏘고, 간격을 짧게 잡아서
	// 날아가는 탄들이 촘촘하게 이어져 실선처럼 보이게 한다.
	Vector leftDir(1.f, 1.f);
	leftDir.Normalize();
	scene->CreateBullet(Vector(0.f, GetPos().y), BulletType::Enemy, leftDir, 300.f);

	Vector rightDir(-1.f, 1.f);
	rightDir.Normalize();
	scene->CreateBullet(Vector((float)GWinSizeX, GetPos().y), BulletType::Enemy, rightDir, 300.f);
}

void Boss::shootConvergingBurst()
{
	if (_isDead)
		return;

	GameScene* scene = Game::GetInstance().GetScene();
	if (scene == nullptr)
		return;

	_attackPoseTimer = 0.3f;

	// 원 둘레 방향마다 3발씩(느리게/그대로/빠르게) 동시에 나가서 전부 _convergingSpeed로 수렴한다.
	// _circleAngle/_circleRotationSpeed를 그대로 재사용해서, 쏠 때마다 살짝씩 회전하는 것도 가능하다.
	constexpr float convergeDuration = 0.4f;	// 이 시간 동안 목표 속도까지 가속/감속
	float accel = _convergingSpeedSpread / convergeDuration;
	int32 count = (int32)(_circleShotCount * _bulletCountMul);

	float angleOffset = DegreeToRadian(_convergingAngleSpread);

	for (int32 i = 0; i < count; ++i)
	{
		float radian = DegreeToRadian(_circleAngle + i * (360.f / count));

		Vector dirSlow(cosf(radian - angleOffset), sinf(radian - angleOffset));
		Vector dirMid(cosf(radian), sinf(radian));
		Vector dirFast(cosf(radian + angleOffset), sinf(radian + angleOffset));

		// 느리게 시작 -> 가속
		scene->CreateBullet(GetPos(), BulletType::Enemy, dirSlow, _convergingSpeed - _convergingSpeedSpread, false, 180.f, accel,
			0.f, 0.f, BulletRedirectMode::None, L"", -1.f, false, _convergingSpeed);
		// 처음부터 목표 속도
		scene->CreateBullet(GetPos(), BulletType::Enemy, dirMid, _convergingSpeed, false, 180.f, 0.f,
			0.f, 0.f, BulletRedirectMode::None, L"", -1.f, false, _convergingSpeed);
		// 빠르게 시작 -> 감속
		scene->CreateBullet(GetPos(), BulletType::Enemy, dirFast, _convergingSpeed + _convergingSpeedSpread, false, 180.f, -accel,
			0.f, 0.f, BulletRedirectMode::None, L"", -1.f, false, _convergingSpeed);
	}

	_circleAngle += _circleRotationSpeed;
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

	// 특정 페이즈(_burstTextureAltPhaseIndex)에서만 다른 텍스처/콜라이더를 쓴다. Alt 텍스처는 방향 회전을
	// 지원하지 않는 고정 이미지라 faceDirection은 기본 텍스처(_burstTextureKey)에만 적용한다.
	wstring textureKey = _burstTextureKey;
	float colliderSize = _bulletsFaceDirection ? _faceDirectionColliderSize : -1.f;
	bool faceDirection = _bulletsFaceDirection;
	if (_curPhaseIndex == _burstTextureAltPhaseIndex && !_burstTextureKeyAlt.empty())
	{
		textureKey = _burstTextureKeyAlt;
		colliderSize = _burstColliderSizeAlt;
		faceDirection = false;
	}
	scene->FireStraight(GetPos(), BulletType::Enemy, _burstDir, speed, textureKey, colliderSize, faceDirection);

	_burstShotsRemaining--;
	if (_burstShotsRemaining <= 0)
	{
		TimeManager::GetInstance().Remove(_burstShootTimerId);
	}
}

void Boss::shootTelegraphBullet()
{
	// 예고 판정: 플레이어 위치가 아니라, 화면 상단 좌/우 고정 지점 2곳에 경고 표시 후
	// 일정 시간 뒤 그 자리를 중심으로 원형탄을 터뜨린다.
	Vector warnPosLeft(100.f, 150.f);
	Vector warnPosRight(GWinSizeX - 100.f, 150.f);

	Game::GetInstance().GetScene()->CreateEffect(warnPosLeft);
	Game::GetInstance().GetScene()->CreateEffect(warnPosRight);

	_telegraphTimerId = TimeManager::GetInstance().AddTimer([warnPosLeft, warnPosRight, countMul = _bulletCountMul, speedMul = _bulletSpeedMul]()
	{
		// this를 캡처하지 않는다: Boss는 풀이 아니라 new/delete로 관리되므로,
		// 이 타이머가 delete된 Boss를 가리키는 채로 남아있을 수 있다 (use-after-free 방지).
		// 배율은 값으로 미리 복사해서 캡처한다.
		GameScene* scene = Game::GetInstance().GetScene();
		if (scene == nullptr)
			return;

		scene->FireCircle(warnPosLeft, BulletType::Enemy, (int32)(10 * countMul), 250.f * speedMul);
		scene->FireCircle(warnPosRight, BulletType::Enemy, (int32)(10 * countMul), 250.f * speedMul);
	}, 1.8f, false);
}

void Boss::shootSpreadBullet()
{
	if (_isDead) return;
    GameScene* scene = Game::GetInstance().GetScene();
    if (scene == nullptr) return;

    static random_device rd;
    static mt19937 gen(rd());
    uniform_real_distribution<float> angleDist(-_spreadAngle, _spreadAngle);
    uniform_real_distribution<float> speedDist(_spreadMinSpeed, _spreadMaxSpeed);

    float baseAngle = RadianToDegree(atan2f(_spreadBaseDir.y, _spreadBaseDir.x));
    float radian = DegreeToRadian(baseAngle + angleDist(gen));
    Vector dir(cosf(radian), sinf(radian));

    scene->FireStraight(GetPos(), BulletType::Enemy, dir, speedDist(gen) * _bulletSpeedMul, L"SpreadBulletFire", 10.f);

    _spreadShotsRemaining--;
    if (_spreadShotsRemaining <= 0)
    {
        TimeManager::GetInstance().Remove(_spreadShootTimerId);
    }
}


