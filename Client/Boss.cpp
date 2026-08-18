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
#include "Laser.h"
#include "BossIllusion.h"
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

	// BorderAimedBurst도 마찬가지.
	bool ContainsBorderAimedBurst(const vector<TimelineStep>& timeline)
	{
		for (const TimelineStep& step : timeline)
		{
			if (step.pattern == BossPatternType::BorderAimedBurst)
				return true;
		}
		return false;
	}

	bool ContainsLeavatein(const vector<TimelineStep>& timeline)
	{
		for(const TimelineStep& step : timeline)
		{
			if(step.pattern == BossPatternType::Leavatein)
				return true;
		}
		return false;
	}

	// Kagome도 마찬가지로, 타임라인에 있는지만 확인해서 전용 상태머신을 켠다.
	bool ContainsKagome(const vector<TimelineStep>& timeline)
	{
		for (const TimelineStep& step : timeline)
		{
			if (step.pattern == BossPatternType::Kagome)
				return true;
		}
		return false;
	}

	// LoveMaze도 마찬가지.
	bool ContainsLoveMaze(const vector<TimelineStep>& timeline)
	{
		for (const TimelineStep& step : timeline)
		{
			if (step.pattern == BossPatternType::LoveMaze)
				return true;
		}
		return false;
	}

	// StarbowBreak도 마찬가지.
	bool ContainsStarbowBreak(const vector<TimelineStep>& timeline)
	{
		for (const TimelineStep& step : timeline)
		{
			if (step.pattern == BossPatternType::StarbowBreak)
				return true;
		}
		return false;
	}

	// PastClock도 마찬가지.
	bool ContainsPastClock(const vector<TimelineStep>& timeline)
	{
		for (const TimelineStep& step : timeline)
		{
			if (step.pattern == BossPatternType::PastClock)
				return true;
		}
		return false;
	}

	// QED도 마찬가지.
	bool ContainsQED(const vector<TimelineStep>& timeline)
	{
		for (const TimelineStep& step : timeline)
		{
			if (step.pattern == BossPatternType::QED)
				return true;
		}
		return false;
	}

	// 카고메 카고메(6페이즈) 튜닝 상수.
	constexpr float KAGOME_GRID_SPACING = 40.f;			// 격자탄 한 알 사이 간격
	constexpr float KAGOME_LINE_START_STAGGER = 0.1f;		// 가로/세로 웨이브에서 줄마다 시작이 밀리는 간격
	constexpr float KAGOME_BULLET_SPAWN_INTERVAL = 0.05f;	// 한 줄 안에서 탄이 한 알씩 생기는 간격
	constexpr float KAGOME_GRID_BULLET_RADIUS = 7.f;		// 격자탄 콜라이더(KagomeGridBulletGreen 16x16 기준)
	constexpr float KAGOME_GRID_BULLET_LIFETIME = 6.0f;	// 이 시간이 지나면 위치와 무관하게 자동 소멸(풀 고갈 방지)
	constexpr float KAGOME_BIG_BULLET_SPEED = 220.f;
	constexpr float KAGOME_BIG_BULLET_RADIUS = 35.f;		// 격자탄(13.5)과 정수 반지름이 겹치지 않도록 충분히 큰 값
	constexpr float KAGOME_DISRUPT_RADIUS = 45.f;			// 큰 탄 주변 이 반경 안의 격자탄이 흐트러진다
	constexpr float KAGOME_SCATTER_SPEED = 130.f;			// 흐트러진 격자탄이 밀려나가는 속도
	constexpr float KAGOME_SINGLE_BULLET_ANGLE = 20.f;		// 단발이 아래 기준 좌/우로 무작위로 기울어지는 각도

	// 사랑의 미로(7페이즈) 튜닝 상수.
	constexpr float LOVE_MAZE_GAP_WIDTH = 20.f;			// 항상 비어있는 구간의 각도 폭
	constexpr int32 LOVE_MAZE_SPIRAL_ARM_COUNT = 1;
	constexpr float LOVE_MAZE_SPIRAL_INTERVAL = 0.025f;		// 0.05 -> 0.025 (더 자주 발사해서 더 촘촘하게)
	// 0.025초 간격 x 9도 = 초당 360도 = 대략 1초에 한 바퀴(회전 속도는 그대로 유지).
	constexpr float LOVE_MAZE_SPIRAL_ROTATION_SPEED = 9.f;	// 발사마다 팔이 회전하는 각도
	constexpr float LOVE_MAZE_SPIRAL_SPEED = 110.f;		// 220 -> 110 (속도 절반)
	constexpr int32 LOVE_MAZE_CIRCLE_COUNT = 48;			// 24 -> 48 (탄환 갯수 2배)
	constexpr float LOVE_MAZE_CIRCLE_INTERVAL = 0.8f;
	constexpr float LOVE_MAZE_CIRCLE_ROTATION_SPEED = 15.f;	// 발사마다 링이 회전해서 겹겹이 쌓이는 효과
	constexpr float LOVE_MAZE_CIRCLE_SPEED = 100.f;		// 200 -> 100 (속도 절반)
	constexpr float LOVE_MAZE_SPIRAL_BULLET_RADIUS = 7.f;	// LoveMazeSpiralGreen 14x16 기준
	constexpr float LOVE_MAZE_CIRCLE_BULLET_RADIUS = 7.f;	// LoveMazeCircleBlue 16x16 기준

	// 스타보우 브레이크(8페이즈) 튜닝 상수.
	constexpr int32 STARBOW_BULLETS_PER_LINE = 15;			// 줄 하나당 탄 개수
	constexpr float STARBOW_LINE_SPACING = 40.f;			// 줄 안에서 탄 사이 간격(대각선/가로/세로 공통)
	constexpr float STARBOW_DIAGONAL_LINE_OFFSET = 100.f;	// 대각선 6줄끼리 서로 떨어진 간격(탄 간격과는 별개)
	constexpr float STARBOW_DIAGONAL_Y_SHIFT = -100.f;		// 대각선 대형 전체를 화면 위쪽으로 옮기는 보정값(음수=위로)
	constexpr float STARBOW_BULLET_SPAWN_INTERVAL = 0.03f;	// 한 줄 안에서 탄이 한 알씩 생기는 간격
	constexpr float STARBOW_WAVE_PAUSE = 2.5f;				// 한 웨이브의 모든 줄이 다 스폰된 뒤 다음 웨이브까지 대기 시간
	constexpr float STARBOW_RISE_DURATION = 1.0f;			// 스폰 직후 위로 떠오르는 시간
	constexpr float STARBOW_RISE_HEIGHTS[3] = { 40.f, 80.f, 120.f };	// 떠오르는 높이(위 방향 이동 거리), 탄마다 무작위
	constexpr float STARBOW_FALL_ACCEL_SLOW = 100.f;		// 낙하 가속도(느림)
	constexpr float STARBOW_FALL_ACCEL_FAST = 150.f;		// 낙하 가속도(빠름). 느림/빠름 중 탄마다 무작위.
	constexpr float STARBOW_SPAWN_X_JITTER = 15.f;			// 스폰 x좌표에 주는 무작위 지터(±). 격자처럼 딱 맞는 느낌을 깨기 위함.

	// 과거를 새기는 시계(9페이즈) 튜닝 상수.
	constexpr float PAST_CLOCK_FAN_INTERVAL = 0.7f;			// 위/아래 부채꼴 공유 발사 주기
	constexpr int32 PAST_CLOCK_UPPER_FAN_COUNT = 30;			// 위쪽 270도 부채꼴 탄수(9도 간격)
	constexpr float PAST_CLOCK_UPPER_FAN_SPREAD = 270.f;
	constexpr float PAST_CLOCK_UPPER_FAN_SPEED = 200.f;
	constexpr int32 PAST_CLOCK_LOWER_FAN_COUNT = 16;			// 아래쪽 120도 조준 부채꼴 탄수
	constexpr float PAST_CLOCK_LOWER_FAN_SPREAD = 120.f;
	constexpr float PAST_CLOCK_LOWER_FAN_SPEED = 100.f;
	constexpr float PAST_CLOCK_FAN_BULLET_RADIUS = 14.f;		// IllusionFanRed(32x32) 기준 콜라이더

	constexpr int32 PAST_CLOCK_PROPELLER_BLADE_COUNT = 4;		// 프로펠러 날개 개수(90도 간격)
	constexpr float PAST_CLOCK_PROPELLER_BLADE_LENGTH = 200.f;	// 날개 길이
	constexpr int32 PAST_CLOCK_PROPELLER_SEGMENT_COUNT = 32;	// 날개 하나당 콜라이더(LaserSegment) 개수
	constexpr int32 PAST_CLOCK_PROPELLER_SEGMENT_RADIUS = 4;	// 콜라이더 반지름
	// 회전/이동 속도는 요청 스펙에 없어서 임시로 정한 값 — 플레이테스트하면서 튜닝 필요.
	constexpr float PAST_CLOCK_PROPELLER_ANGULAR_SPEED = 45.f;	// 초당 회전 각도(도)
	constexpr float PAST_CLOCK_PROPELLER_MOVE_SPEED = 80.f;		// 초당 x축 이동 속도
	constexpr float PAST_CLOCK_PROPELLER_MARGIN = 50.f;		// 화면 양 끝에서 이만큼 남기고 멈춤(x=50~550)
	constexpr float PAST_CLOCK_PROPELLER_REST_DURATION = 2.0f;	// 끝에 도달해서 회전까지 멈추고 쉬는 시간

	// Q.E.D. 495년의 파문(10페이즈) 튜닝 상수.
	constexpr int32 QED_CIRCLE_BULLET_COUNT = 50;			// 원형탄 탄수(고정)
	constexpr float QED_TIER_BASE_HP = 500.f;				// 이 HP에서 시작(0단계: 속도/주기 기본값)
	constexpr float QED_HP_TIER_SIZE = 100.f;				// 이 HP만큼 깎일 때마다 한 단계씩
	constexpr float QED_BASE_SPEED = 150.f;					// 0단계 속도
	constexpr float QED_SPEED_STEP = 20.f;					// 단계마다 증가하는 속도
	constexpr float QED_BASE_INTERVAL = 2.0f;				// 0단계 발사 주기
	constexpr float QED_INTERVAL_STEP = 0.1f;				// 단계마다 감소하는 발사 주기
	constexpr float QED_MIN_INTERVAL = 0.2f;				// 발사 주기 안전 하한
	constexpr float QED_TOP_SPAWN_Y = 80.f;					// 최초 이후 웨이브가 터지는 화면 상단 y좌표
	constexpr float QED_BULLET_RADIUS = 7.f;				// QEDPetalBlue(16x16) 기준 콜라이더
	constexpr float STARBOW_BULLET_RADIUS = 7.f;			// Starbow 계열(16x16, etama3.png에서 크롭) 텍스처 기준 콜라이더.
	// 색깔별 텍스처 키. etama3.png(y=32 행)의 3/14/12/9/7/5번째 탄을 순서대로 추출한 6색.
	const wchar_t* STARBOW_COLOR_KEYS[6] = { L"StarbowRed", L"StarbowYellow", L"StarbowLime", L"StarbowCyan", L"StarbowBlue", L"StarbowMagenta" };
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
			float convergingAngleSpread,
			int32 illusionPhaseIndex)
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
	_illusionPhaseIndex = illusionPhaseIndex;

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
	if (ContainsBorderAimedBurst(_phases[_curPhaseIndex].timeline))
	{
		initBorderMarkers();
		_borderBurstTimerId = TimeManager::GetInstance().AddTimer([this]() { shootBorderAimedBurst(); }, _borderBurstInterval, true);
	}
	if (ContainsLeavatein(_phases[_curPhaseIndex].timeline))
	{
		_leavateinActive = true;
		SetPos(Vector(GWinSizeX * 0.5f, 50.f));	// 레바테인은 항상 가운데(300,50)에서 시작
		beginLeavateinAction((rand() % 2 == 0) ? LeavateinAction::SweepLeftStart : LeavateinAction::SweepRightStart);
		_leavateinBulletTimerId = TimeManager::GetInstance().AddTimer([this]() { shootLeavateinBullets(); }, 0.2f, true);
	}
	if (ContainsKagome(_phases[_curPhaseIndex].timeline))
	{
		startKagomeSpellcard();
	}
	if (ContainsLoveMaze(_phases[_curPhaseIndex].timeline))
	{
		startLoveMazeSpellcard();
	}
	if (ContainsStarbowBreak(_phases[_curPhaseIndex].timeline))
	{
		startStarbowBreak();
	}
	if (ContainsPastClock(_phases[_curPhaseIndex].timeline))
	{
		startPastClockSpellcard();
	}
	if (ContainsQED(_phases[_curPhaseIndex].timeline))
	{
		startQEDSpellcard();
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
	TimeManager::GetInstance().Remove(_borderBurstTimerId);
	TimeManager::GetInstance().Remove(_telegraphTimerId);
	TimeManager::GetInstance().Remove(_burstShootTimerId);
	TimeManager::GetInstance().Remove(_spreadShootTimerId);
	_leavateinActive = false;
	TimeManager::GetInstance().Remove(_leavateinBulletTimerId);
	if (_leavateinLaser != nullptr)
	{
		_leavateinLaser->Destroy();
		_leavateinLaser = nullptr;
	}

	TimeManager::GetInstance().Remove(_illusionWanderTimerId);

	stopKagomeSpellcard();
	stopLoveMazeSpellcard();
	stopStarbowBreak();
	stopPastClockSpellcard();
	stopQEDSpellcard();

	// 5페이즈 도중 보스가 죽으면 아직 살아있는 분신들도 같이 정리한다.
	// Boss는 분신 포인터를 직접 들고 있지 않는다 — 분신이 플레이어 총알에 먼저 죽으면
	// scene이 그 인스턴스를 delete하므로, Boss가 포인터를 들고 있으면 댕글링 위험이 있다.
	// 대신 scene에 "Boss 레이어에 남은 분신 전부 정리해줘"라고 요청한다.
	GameScene* scene = Game::GetInstance().GetScene();
	if (scene != nullptr)
	{
		scene->ClearBossIllusions();
	}
}

void Boss::Update(float deltaTime)
{
	Super::Update(deltaTime);

	if (_leavateinActive)
	{
		updateLeavatein(deltaTime);
	}
	if (_borderBurstTimerId != -1)
	{
		updateBorderMarkers(deltaTime);
	}
	if (_kagomeActive)
	{
		updateKagomeGrid(deltaTime);
		updateKagomeBurst(deltaTime);
		updateKagomeDisruption(deltaTime);
	}
	if (_starbowActive)
	{
		updateStarbowBreak(deltaTime);
	}
	if (_pastClockActive)
	{
		updatePastClockPropellers(deltaTime);
	}
	if (_qedActive)
	{
		updateQED(deltaTime);
	}
	// 이 페이즈 동안엔 랜덤 이동 타이머가 뭘 정해놨든 무시하고 매 프레임 중앙을 목표로 고정한다.
	if (_curPhaseIndex == _fixedPosPhaseIndex)
	{
		_moveTargetPos = Vector(GWinSizeX * 0.5f, 150.f);
	}
	// 5페이즈 동안은 분신들과 같은 방식으로, 배정된 자리(_illusionBossSlot) 주변에서 배회한다.
	if (_curPhaseIndex == _illusionPhaseIndex)
	{
		_moveTargetPos = _illusionWanderTargetPos;
	}
	// 7페이즈(사랑의 미로) 동안은 Spiral/Circle이 같은 원점에서 나가야 대칭이 유지되므로 중앙 고정.
	// _fixedPosPhaseIndex는 1페이즈가 이미 쓰고 있어서, 이 페이즈 전용 플래그(_loveMazeActive)를 그대로 쓴다.
	// 다른 고정 페이즈들과 달리 화면 상단이 아니라 플레이 화면 전체의 정중앙에 고정한다.
	if (_loveMazeActive)
	{
		_moveTargetPos = Vector(GWinSizeX * 0.5f, GWinSizeY * 0.5f);
	}
	// 9페이즈(과거를 새기는 시계) 동안은 화면 상단 중앙 고정.
	if (_pastClockActive)
	{
		_moveTargetPos = Vector(GWinSizeX * 0.5f, 150.f);
	}
	// 10페이즈(Q.E.D.) 동안도 화면 상단 중앙 고정.
	if (_qedActive)
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
		// Spiral/Cross/ConvergingBurst/BorderAimedBurst는 연속 타이머가 따로 쏘고 있으므로 여기서는 건너뛴다.
		if (timeline[_timelineIndex].pattern != BossPatternType::Spiral &&
			timeline[_timelineIndex].pattern != BossPatternType::Cross &&
			timeline[_timelineIndex].pattern != BossPatternType::ConvergingBurst &&
			timeline[_timelineIndex].pattern != BossPatternType::BorderAimedBurst&&
			timeline[_timelineIndex].pattern != BossPatternType::Leavatein &&
			timeline[_timelineIndex].pattern != BossPatternType::Kagome &&
			timeline[_timelineIndex].pattern != BossPatternType::LoveMaze &&
			timeline[_timelineIndex].pattern != BossPatternType::StarbowBreak &&
			timeline[_timelineIndex].pattern != BossPatternType::PastClock &&
			timeline[_timelineIndex].pattern != BossPatternType::QED)
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

	// BorderAimedBurst가 활성화된 동안, 6개의 마법진을 각자 위치에 그려준다.
	if (_borderBurstTimerId != -1)
	{
		Texture* magicCircle = ResourceManager::GetInstance().GetTexture(L"MagicCircle");
		if (magicCircle)
		{
			for (int32 i = 0; i < 6; ++i)
			{
				magicCircle->Render(hdc, getBorderMarkerPos(i));
			}
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

	// 5페이즈(분신 소환 페이즈)를 벗어나면, 아직 살아있는 분신들을 정리한다.
	// (본체가 죽었을 때의 Destroy()와 같은 정리를, 페이즈 전환 시점에도 해줘야 한다.)
	if (_curPhaseIndex == _illusionPhaseIndex)
	{
		TimeManager::GetInstance().Remove(_illusionWanderTimerId);
		Game::GetInstance().GetScene()->ClearBossIllusions();
	}

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
	TimeManager::GetInstance().Remove(_borderBurstTimerId);
	_borderBurstTimerId = -1;	// Remove()가 값으로 ID를 받아서 여기서 직접 리셋해야, 이전 페이즈의 마법진 렌더링/갱신 조건(!= -1)이 꺼진다.
	if (ContainsBorderAimedBurst(_phases[_curPhaseIndex].timeline))
	{
		initBorderMarkers();
		_borderBurstTimerId = TimeManager::GetInstance().AddTimer([this]() { shootBorderAimedBurst(); }, _borderBurstInterval, true);
	}
	_leavateinActive = false;
	TimeManager::GetInstance().Remove(_leavateinBulletTimerId);
	if(_leavateinLaser != nullptr)
	{
		_leavateinLaser->Destroy();
		_leavateinLaser = nullptr;
	}
	if (ContainsLeavatein(_phases[_curPhaseIndex].timeline))
	{
		_leavateinActive = true;
		SetPos(Vector(GWinSizeX * 0.5f, 50.f));	// 레바테인은 항상 가운데(300,50)에서 시작
		beginLeavateinAction((rand() % 2 == 0) ? LeavateinAction::SweepLeftStart : LeavateinAction::SweepRightStart);
		_leavateinBulletTimerId = TimeManager::GetInstance().AddTimer([this]() { shootLeavateinBullets(); }, 0.2f, true);
	}

	if (_curPhaseIndex == _illusionPhaseIndex)
	{
		spawnIllusionClones();
	}

	stopKagomeSpellcard();
	if (ContainsKagome(_phases[_curPhaseIndex].timeline))
	{
		startKagomeSpellcard();
	}

	stopLoveMazeSpellcard();
	if (ContainsLoveMaze(_phases[_curPhaseIndex].timeline))
	{
		startLoveMazeSpellcard();
	}

	stopStarbowBreak();
	if (ContainsStarbowBreak(_phases[_curPhaseIndex].timeline))
	{
		startStarbowBreak();
	}

	stopPastClockSpellcard();
	if (ContainsPastClock(_phases[_curPhaseIndex].timeline))
	{
		startPastClockSpellcard();
	}

	stopQEDSpellcard();
	if (ContainsQED(_phases[_curPhaseIndex].timeline))
	{
		startQEDSpellcard();
	}
}

void Boss::shootBullet(BossPatternType pattern)
{
	_attackPoseTimer = 0.3f;

	switch(pattern)
	{
		case BossPatternType::Fan :
		{
			// 5페이즈(IllusionBurst 경유)에서는 본체 몫 색상(_fanTextureKeyAlt)을 쓴다.
			bool useIllusionAlt = (_curPhaseIndex == _illusionPhaseIndex);
			wstring fanTexture = useIllusionAlt ? _fanTextureKeyAlt : _fanTextureKey;
			// IllusionFan 텍스처는 32x32라, 자동 콜라이더(GetSizeX()-3=29)를 쓰면 스프라이트보다 훨씬 커진다.
			// 30x30 텍스처에 13.5를 쓴 CircleBulletYellow 사례처럼 절반 정도로 명시해준다.
			float fanColliderSize = useIllusionAlt ? 14.f : (fanTexture.empty() ? -1.f : _faceDirectionColliderSize);
			Game::GetInstance().GetScene()->FireFan(GetPos(), BulletType::Enemy, Vector(0,1), _fanAngleSpread,
				(int32)(_fanShotCount * _bulletCountMul), 300.f * _bulletSpeedMul,
				fanTexture, fanColliderSize, useIllusionAlt ? false : _bulletsFaceDirection);
			break;
		}
		case BossPatternType::Circle :
		{
			// 5페이즈에서는 _circleTextureKeyAlt(고리형 탄)를 쓴다.
			bool useIllusionAlt = (_curPhaseIndex == _illusionPhaseIndex);
			wstring circleTexture = useIllusionAlt ? _circleTextureKeyAlt : _circleTextureKey;
			// IllusionCircle은 16x16인데 자동 콜라이더(GetSizeX()-3=13)는 CircleBulletYellow(30x30->13.5)
			// 등 다른 텍스처들이 쓰는 "절반 정도" 비율(약 0.45배)보다 훨씬 크다. 같은 비율로 맞춘다.
			float circleColliderSize = useIllusionAlt ? 7.f : _circleColliderSize;
			Game::GetInstance().GetScene()->FireCircle(GetPos(), BulletType::Enemy,
				(int32)(_circleShotCount * _bulletCountMul), 300.f * _bulletSpeedMul,
				0.f, 0.f, BulletRedirectMode::None, circleTexture, circleColliderSize, useIllusionAlt ? false : _bulletsFaceDirection, _circleAngle);
			_circleAngle += _circleRotationSpeed;	// 다음 Circle은 이만큼 회전된 각도에서 시작 (0이면 매번 그대로)
			break;
		}
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
		case BossPatternType::BorderAimedBurst :
			// Update()에서 이미 걸러내고 연속 타이머(_borderBurstTimerId)로 처리하므로 여기선 아무것도 안 함.
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
		case BossPatternType::Leavatein :
			// TODO: Sweep/Slide 로직은 Update()에서 처리 예정
			break;
		case BossPatternType::IllusionBurst :
		{
			// Circle/Fan을 동시에 쏘지 않고, 매 사이클마다 둘 중 하나만 무작위로 골라 쏜다.
			BossPatternType chosen = (rand() % 2 == 0) ? BossPatternType::Circle : BossPatternType::Fan;
			shootBullet(chosen);
			break;
		}
		case BossPatternType::Kagome :
			// Update()에서 이미 걸러내고 전용 상태머신(updateKagomeGrid/updateKagomeBurst)으로 처리하므로 여기선 아무것도 안 함.
			break;
		case BossPatternType::LoveMaze :
			// Update()에서 이미 걸러내고 전용 타이머(shootLoveMazeSpiral/shiftLoveMazeGapAndFireCircle)로 처리하므로 여기선 아무것도 안 함.
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

Vector Boss::getBorderCornerPos(int32 cornerIndex) const
{
	switch (((cornerIndex % 4) + 4) % 4)
	{
		case 0: return Vector(0.f, 0.f);
		case 1: return Vector((float)GWinSizeX, 0.f);
		case 2: return Vector((float)GWinSizeX, (float)GWinSizeY);
		default: return Vector(0.f, (float)GWinSizeY);
	}
}

Vector Boss::getBorderMarkerPos(int32 markerIndex) const
{
	const BorderMarker& m = _borderMarkers[markerIndex];
	Vector from = getBorderCornerPos(m.fromCorner);
	Vector to = getBorderCornerPos(m.toCorner);
	// 인접 코너는 항상 X축 또는 Y축 중 한쪽 좌표만 다르므로, 선형보간만으로 축 이동이 자연히 결정된다.
	float t = (m.segmentTime < _borderGlideDuration) ? (m.segmentTime / _borderGlideDuration) : 1.f;
	return from + (to - from) * t;
}

void Boss::initBorderMarkers()
{
	for (int32 i = 0; i < 6; ++i)
	{
		bool groupB = (i >= 3);
		int32 startCorner = groupB ? 2 : 0;	// 그룹A=(0,0), 그룹B=대각선 반대 코너
		int32 dir = (rand() % 2 == 0) ? 1 : -1;

		_borderMarkers[i].fromCorner = startCorner;
		_borderMarkers[i].toCorner = startCorner + dir;
		// 3번(인덱스2)/6번(인덱스5)만 시작 타이머를 0.2초 앞당겨서 리더와 살짝 어긋나 보이게 한다.
		_borderMarkers[i].segmentTime = (i == 2 || i == 5) ? 0.2f : 0.f;
		_borderMarkers[i].aimAtPlayer = (i == 0 || i == 3);
	}
}

void Boss::updateBorderMarkers(float deltaTime)
{
	float segmentDuration = _borderGlideDuration + _borderPauseDuration;

	for (int32 i = 0; i < 6; ++i)
	{
		BorderMarker& m = _borderMarkers[i];
		m.segmentTime += deltaTime;
		if (m.segmentTime >= segmentDuration)
		{
			m.segmentTime -= segmentDuration;
			m.fromCorner = m.toCorner;
			// 코너에 도착할 때마다 독립적으로 시계/반시계 방향을 다시 랜덤으로 고른다.
			int32 dir = (rand() % 2 == 0) ? 1 : -1;
			m.toCorner = m.fromCorner + dir;
		}
	}
}

void Boss::shootBorderAimedBurst()
{
	if (_isDead)
		return;

	GameScene* scene = Game::GetInstance().GetScene();
	if (scene == nullptr)
		return;

	_attackPoseTimer = 0.3f;

	const Vector fixedTarget(300.f, 400.f);
	Player* player = scene->GetPlayer();

	for (int32 i = 0; i < 6; ++i)
	{
		Vector pos = getBorderMarkerPos(i);
		bool aimAtPlayer = _borderMarkers[i].aimAtPlayer;
		// 조준 대상에 따라 탄 색을 구분한다: 플레이어 조준=빨강, 화면 중앙 조준=파랑.
		Vector target = (aimAtPlayer && player != nullptr) ? player->GetPos() : fixedTarget;
		wstring textureKey = aimAtPlayer ? L"OrbRed" : L"OrbBlue";

		Vector dir = target - pos;
		dir.Normalize();

		scene->CreateBullet(pos, BulletType::Enemy, dir, _borderBulletSpeed, false, 180.f, 0.f, 0.f, 0.f, BulletRedirectMode::None, textureKey);
	}
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

// Reposition(칼날 없이 이동)이 끝난 뒤 어느 위치에서 다음 액션을 시작해야 하는지.
// Sweep 두 종류는 둘 다 가운데에서 시작하고, Slide 두 종류는 출발 쪽 화면 끝에서 시작한다.
Vector Boss::getLeavateinLaunchPos(LeavateinAction action) const
{
	switch (action)
	{
		case LeavateinAction::SweepLeftStart:
		case LeavateinAction::SweepRightStart:
			return Vector(GWinSizeX * 0.5f, 50.f);
		case LeavateinAction::SlideToLeft:
			return Vector((float)GWinSizeX, 50.f);	// 오른쪽 끝에서 출발해서 왼쪽으로
		case LeavateinAction::SlideToRight:
		default:
			return Vector(0.f, 50.f);					// 왼쪽 끝에서 출발해서 오른쪽으로
	}
}

// 방금 끝낸 액션(_leavateinLastAction)과 완전히 같은 것만 제외하고 나머지 3개 중 랜덤으로 고른다.
void Boss::pickNextLeavateinAction()
{
	LeavateinAction all[4] = { LeavateinAction::SweepLeftStart, LeavateinAction::SweepRightStart,
							    LeavateinAction::SlideToLeft, LeavateinAction::SlideToRight };
	LeavateinAction candidates[3];
	int32 count = 0;
	for (LeavateinAction action : all)
	{
		if (action != _leavateinLastAction)
			candidates[count++] = action;
	}
	_leavateinPendingAction = candidates[rand() % 3];
}

// Sweep/Slide가 끝나면 곧바로 다음 액션으로 넘어가지 않고, 칼날을 지운 채 0.5초 멈춰서
// 화면에 남은 탄환이 빠질 시간을 준다. 0.5초 뒤 pickNextLeavateinAction()+startLeavateinReposition()으로 이어진다.
void Boss::startLeavateinPause()
{
	if (_leavateinLaser != nullptr)
	{
		_leavateinLaser->Destroy();
		_leavateinLaser = nullptr;
	}

	_leavateinPhase = LeavateinPhase::Pause;
	_leavateinStateTimer = 0.f;
}

// _leavateinPendingAction의 시작 위치로 칼날 없이 이동을 시작한다(약 1초).
// 이미 그 위치라면(가운데->가운데인 Sweep<->반대Sweep 전환) 이동 없이 바로 다음 액션을 시작한다.
void Boss::startLeavateinReposition()
{
	if (_leavateinLaser != nullptr)
	{
		_leavateinLaser->Destroy();
		_leavateinLaser = nullptr;
	}

	_leavateinMoveFrom = GetPos();
	_leavateinMoveTo = getLeavateinLaunchPos(_leavateinPendingAction);
	_leavateinStateTimer = 0.f;

	if ((_leavateinMoveTo - _leavateinMoveFrom).Length() < 1.f)
	{
		beginLeavateinAction(_leavateinPendingAction);
		return;
	}

	_leavateinPhase = LeavateinPhase::Reposition;
}

// 칼날을 새로 스폰하고 Sweep 또는 Slide 상태로 진입한다.
void Boss::beginLeavateinAction(LeavateinAction action)
{
	_leavateinLastAction = action;
	_leavateinStateTimer = 0.f;

	if (action == LeavateinAction::SweepLeftStart || action == LeavateinAction::SweepRightStart)
	{
		bool startFromLeft = (action == LeavateinAction::SweepLeftStart);
		float startAngle = startFromLeft ? 90.f : 270.f;		// 90=왼쪽, 270=오른쪽 (이 프로젝트 각도 기준)
		float angularSpeed = startFromLeft ? 90.f : -90.f;		// 왼쪽 시작->증가(왼쪽->위->오른쪽->아래), 오른쪽 시작->감소

		_leavateinLaser = Game::GetInstance().GetScene()->CreateLaser(GetPos(), startAngle, 750.f, 50, 11);
		_leavateinLaser->_bladeTexture = ResourceManager::GetInstance().GetTexture(L"OrbRed");
		_leavateinLaser->_guardTexture = ResourceManager::GetInstance().GetTexture(L"LeavateinGuard");
		_leavateinLaser->SetAngularSpeed(angularSpeed);
		_leavateinPhase = LeavateinPhase::Sweep;
	}
	else
	{
		_leavateinLaser = Game::GetInstance().GetScene()->CreateLaser(GetPos(), 0.f, 750.f, 50, 11);	// 0도 = 아래 고정
		_leavateinLaser->_bladeTexture = ResourceManager::GetInstance().GetTexture(L"OrbRed");
		_leavateinLaser->_guardTexture = ResourceManager::GetInstance().GetTexture(L"LeavateinGuard");
		_leavateinLaser->SetAngularSpeed(0.f);

		_leavateinMoveFrom = GetPos();
		_leavateinMoveTo = (action == LeavateinAction::SlideToLeft) ? Vector(50.f, 80.f) : Vector((float)GWinSizeX - 50.f, 80.f);
		_leavateinPhase = LeavateinPhase::Slide;
	}
}

// 칼날 위에 9발을 균등 간격으로 배치해서, 칼날 방향에 직교하는 한쪽 방향(고정된 부호)으로 쏜다.
// Sweep/Slide 두 상태 모두에서 계속 호출된다(_leavateinBulletTimerId, 0.2초 간격).
void Boss::shootLeavateinBullets()
{
	if (_leavateinLaser == nullptr)
		return;

	Vector bladeDir = Vector(0, 1).Rotate(DegreeToRadian(_leavateinLaser->GetCurrentAngle()));

	// SweepRightStart/SlideToRight는 SweepLeftStart/SlideToLeft를 좌우로 뒤집은 액션이라,
	// 직교 방향도 같이 뒤집어야 시각적으로 "고정된 한쪽"이 유지된다.
	float mirror = (_leavateinLastAction == LeavateinAction::SweepRightStart ||
					_leavateinLastAction == LeavateinAction::SlideToRight) ? -1.f : 1.f;
	Vector perpDir(-bladeDir.y * mirror, bladeDir.x * mirror);

	Vector pivot = _leavateinLaser->GetPivot();
	float length = _leavateinLaser->GetLength();

	const int32 bulletCount = 9;
	for (int32 i = 0; i < bulletCount; ++i)
	{
		float t = (float)i / (float)(bulletCount - 1);
		Vector pos = pivot + bladeDir * (length * t);
		// faceDirection=true: 회전 프레임(LeavateinBullet_000~180)이 perpDir을 보고 돌아간다.
		Game::GetInstance().GetScene()->CreateBullet(pos, BulletType::Enemy, perpDir, 170.f, false, 180.f, 0.f,
			0.f, 0.f, BulletRedirectMode::None, L"LeavateinBullet", 8.f, true);
	}
}

void Boss::updateLeavatein(float deltaTime)
{
	switch (_leavateinPhase)
	{
		case LeavateinPhase::Sweep:
		{
			_leavateinStateTimer += deltaTime;
			if (_leavateinStateTimer >= 3.0f)
			{
				startLeavateinPause();
			}
			break;
		}
		case LeavateinPhase::Pause:
		{
			_leavateinStateTimer += deltaTime;
			if (_leavateinStateTimer >= 1.0f)
			{
				pickNextLeavateinAction();
				startLeavateinReposition();
			}
			break;
		}
		case LeavateinPhase::Reposition:
		{
			_leavateinStateTimer += deltaTime;
			float t = _leavateinStateTimer / 1.0f;
			if (t > 1.0f) t = 1.0f;

			SetPos(_leavateinMoveFrom + (_leavateinMoveTo - _leavateinMoveFrom) * t);

			if (t >= 1.0f)
			{
				beginLeavateinAction(_leavateinPendingAction);
			}
			break;
		}
		case LeavateinPhase::Slide:
		{
			_leavateinStateTimer += deltaTime;
			float t = _leavateinStateTimer / 2.0f;
			if (t > 1.0f) t = 1.0f;

			Vector pos = _leavateinMoveFrom + (_leavateinMoveTo - _leavateinMoveFrom) * t;
			SetPos(pos);
			if (_leavateinLaser != nullptr)
				_leavateinLaser->SetPivot(pos);

			if (t >= 1.0f)
			{
				startLeavateinPause();
			}
			break;
		}
	}

	// 이 패턴이 보스 위치를 SetPos()로 직접 제어하는 동안, Update() 아래쪽의 랜덤 이동 시스템이
	// 매 프레임 _moveTargetPos를 향해 또 이동시키지 않도록 목표를 현재 위치로 맞춰 무력화한다.
	_moveTargetPos = GetPos();
}

// 화면 상단에 x축으로 4등분한 자리를 만들고, 그중 하나를 무작위로 본체 몫으로 배정한다.
// 나머지 3자리에는 분신이 화면 위에서 내려와 스폰된다 (BossIllusion의 입장 연출).
// 본체가 몇 번째 자리인지 매번 랜덤이라, 대형만 보고는 진짜를 알 수 없다.
void Boss::spawnIllusionClones()
{
	GameScene* scene = Game::GetInstance().GetScene();
	if (scene == nullptr)
		return;

	const float slotXs[4] = { GWinSizeX * 0.2f, GWinSizeX * 0.4f, GWinSizeX * 0.6f, GWinSizeX * 0.8f };
	constexpr float slotY = 120.f;

	// slotOrder를 섞어서 앞의 한 자리를 본체, 나머지 세 자리를 분신 몫으로 나눈다.
	int32 slotOrder[4] = { 0, 1, 2, 3 };
	for (int32 i = 3; i > 0; --i)
	{
		int32 j = rand() % (i + 1);
		int32 tmp = slotOrder[i];
		slotOrder[i] = slotOrder[j];
		slotOrder[j] = tmp;
	}

	_illusionBossSlot = Vector(slotXs[slotOrder[0]], slotY);
	_illusionWanderTargetPos = _illusionBossSlot;

	// 분신들처럼 본체도 배정된 자리 주변에서 배회하도록, 1.5초마다 목표점을 재선정.
	TimeManager::GetInstance().Remove(_illusionWanderTimerId);
	_illusionWanderTimerId = TimeManager::GetInstance().AddTimer([this]()
	{
		float radian = DegreeToRadian((float)(rand() % 360));
		float radius = (float)(rand() % ((int32)_illusionWanderRadius + 1));
		_illusionWanderTargetPos = _illusionBossSlot + Vector(cosf(radian), sinf(radian)) * radius;
	}, 1.5f, true);

	const float shootOffsets[3] = { 0.2f, 0.4f, 0.6f };	// 본체(t=0)와 겹치지 않게 순차적으로 캐스케이드
	// 본체는 _fanTextureKeyAlt(Red)를 쓰므로, 분신 3체는 나머지 색으로 겹치지 않게 배정한다.
	const wstring cloneFanColors[3] = { L"IllusionFanBlue", L"IllusionFanGreen", L"IllusionFanYellow" };
	for (int32 i = 0; i < 3; ++i)
	{
		Vector slotPos(slotXs[slotOrder[i + 1]], slotY);
		scene->CreateBossIllusion(slotPos, _baseKey, 200, shootOffsets[i], cloneFanColors[i]);
	}
}

// ============================================================
// 6페이즈: 카고메 카고메
// 격자탄 웨이브(startKagomeGridWave/advanceKagomeLine)와 큰 탄 리듬(updateKagomeBurst)은
// 서로의 상태를 전혀 참조하지 않는 완전히 독립된 두 상태머신이다. 유일한 접점은
// updateKagomeDisruption()이 큰 탄의 현재 위치 주변 격자탄을 지우는 부분뿐이다.
// ============================================================

void Boss::startKagomeSpellcard()
{
	_kagomeActive = true;

	for (KagomeGridLine& line : _kagomeLines)
		line.active = false;
	for (KagomeBigBulletShadow& shadow : _kagomeBigBulletShadows)
		shadow.active = false;
	_kagomeDiagonalBackslashPending = false;

	// startKagomeGridWave()가 매번 웨이브 종류를 반대로 토글하므로, 반대값에서 시작해야
	// 페이즈 진입 직후 첫 웨이브가 원하는 종류(가로+세로)로 나온다.
	_kagomeGridWaveType = KagomeGridWaveType::Diagonal;
	startKagomeGridWave();

	TimeManager::GetInstance().Remove(_kagomeGridWaveTimerId);
	_kagomeGridWaveTimerId = TimeManager::GetInstance().AddTimer([this]() { startKagomeGridWave(); }, 3.0f, true);

	_kagomeBurstState = KagomeBurstState::Warmup;
	_kagomeBurstTimer = 0.f;
}

void Boss::stopKagomeSpellcard()
{
	_kagomeActive = false;
	TimeManager::GetInstance().Remove(_kagomeGridWaveTimerId);
	_kagomeGridWaveTimerId = -1;
	_kagomeDiagonalBackslashPending = false;

	for (KagomeGridLine& line : _kagomeLines)
		line.active = false;
	for (KagomeBigBulletShadow& shadow : _kagomeBigBulletShadows)
		shadow.active = false;
}

// 3초마다 호출된다: 이전 웨이브 상태를 지우고, 웨이브 종류를 토글해서 새 웨이브를 세팅한다.
void Boss::startKagomeGridWave()
{
	if (_isDead)
		return;

	_kagomeGridWaveType = (_kagomeGridWaveType == KagomeGridWaveType::HorizontalVertical)
		? KagomeGridWaveType::Diagonal : KagomeGridWaveType::HorizontalVertical;

	for (KagomeGridLine& line : _kagomeLines)
		line.active = false;
	_kagomeDiagonalBackslashPending = false;

	if (_kagomeGridWaveType == KagomeGridWaveType::HorizontalVertical)
	{
		// 화면(600x800)을 4등분한 자리에 가로 4줄 + 세로 4줄. 시작점은 줄마다 좌/우, 상/하로 번갈아진다.
		const float rowYs[4] = { 150.f, 300.f, 450.f, 600.f };
		// 오른쪽에서 1~4번: 1번(480)/4번(120)은 그대로. 2번은 360->460(+100)했다가 다시
		// 반대 방향(왼쪽)으로 30 되돌려 430, 3번은 240->140(-100)했다가 반대 방향(오른쪽)으로
		// 30 되돌려 170. 가운데 회피 공간을 넓히되 좌우 끝쪽 두 줄이 너무 붙지 않게 조정.
		const float colXs[4] = { 120.f, 170.f, 430.f, 480.f };

		for (int32 i = 0; i < 4; ++i)
		{
			bool startLeft = (i % 2 == 0);
			KagomeGridLine& line = _kagomeLines[i];
			line.active = true;
			line.pos = Vector(startLeft ? 0.f : (float)GWinSizeX, rowYs[i]);
			line.step = Vector(startLeft ? KAGOME_GRID_SPACING : -KAGOME_GRID_SPACING, 0.f);
			line.remainingBullets = (int32)((float)GWinSizeX / KAGOME_GRID_SPACING) + 1;
			line.startDelay = i * KAGOME_LINE_START_STAGGER;
			line.spawnTimer = 0.f;
		}
		for (int32 i = 0; i < 4; ++i)
		{
			bool startTop = (i % 2 == 0);
			KagomeGridLine& line = _kagomeLines[4 + i];
			line.active = true;
			line.pos = Vector(colXs[i], startTop ? 0.f : (float)GWinSizeY);
			line.step = Vector(0.f, startTop ? KAGOME_GRID_SPACING : -KAGOME_GRID_SPACING);
			line.remainingBullets = (int32)((float)GWinSizeY / KAGOME_GRID_SPACING) + 1;
			line.startDelay = (4 + i) * KAGOME_LINE_START_STAGGER;
			line.spawnTimer = 0.f;
		}
	}
	else
	{
		// "/" 방향 3줄: 화면 아래쪽 모서리를 따라 균등 간격으로 동시에 시작해서 우상향으로 뻗어나간다.
		const float startXs[3] = { 100.f, 300.f, 500.f };
		Vector slashDir(1.f, -1.f);
		slashDir.Normalize();
		int32 slashCount = (int32)(((float)GWinSizeX + (float)GWinSizeY) / KAGOME_GRID_SPACING);

		for (int32 i = 0; i < 3; ++i)
		{
			KagomeGridLine& line = _kagomeLines[i];
			line.active = true;
			line.pos = Vector(startXs[i], (float)GWinSizeY);
			line.step = slashDir * KAGOME_GRID_SPACING;
			line.remainingBullets = slashCount;
			line.startDelay = 0.f;
			line.spawnTimer = 0.f;
		}

		// "\" 3줄은 1초 뒤에 startKagomeBackslashLines()가 세팅한다 (updateKagomeGrid에서 타이밍 관리).
		_kagomeDiagonalBackslashPending = true;
		_kagomeDiagonalBackslashTimer = 1.0f;
	}
}

// Diagonal 웨이브에서 "/" 시작 1초 뒤, "\" 방향 3줄을 세팅한다.
void Boss::startKagomeBackslashLines()
{
	const float startXs[3] = { 100.f, 300.f, 500.f };
	Vector backslashDir(1.f, 1.f);
	backslashDir.Normalize();
	int32 backslashCount = (int32)(((float)GWinSizeX + (float)GWinSizeY) / KAGOME_GRID_SPACING);

	for (int32 i = 0; i < 3; ++i)
	{
		KagomeGridLine& line = _kagomeLines[3 + i];
		line.active = true;
		line.pos = Vector(startXs[i], 0.f);
		line.step = backslashDir * KAGOME_GRID_SPACING;
		line.remainingBullets = backslashCount;
		line.startDelay = 0.f;
		line.spawnTimer = 0.f;
	}
}

// 한 줄을 한 프레임만큼 진행시킨다: 시작 대기 -> 스폰 간격마다 탄 한 알씩 생성 -> 다음 칸으로 이동.
void Boss::advanceKagomeLine(KagomeGridLine& line, float deltaTime)
{
	if (!line.active)
		return;

	if (line.startDelay > 0.f)
	{
		line.startDelay -= deltaTime;
		return;
	}

	line.spawnTimer -= deltaTime;
	if (line.spawnTimer > 0.f)
		return;
	line.spawnTimer += KAGOME_BULLET_SPAWN_INTERVAL;

	// 화면 여유범위(-32~+32) 밖이면 생성을 건너뛴다: Bullet::Update()의 화면밖 삭제 체크가
	// 스폰 다음 프레임에 바로 지워버리는 것을 막기 위함(대각선 줄의 꼬리 부분에서 발생).
	if (line.pos.x >= -32.f && line.pos.x <= (float)GWinSizeX + 32.f &&
		line.pos.y >= -32.f && line.pos.y <= (float)GWinSizeY + 32.f)
	{
		Game::GetInstance().GetScene()->CreateBullet(line.pos, BulletType::Enemy, Vector(0.f, 1.f), 0.f,
			false, 180.f, 0.f, 0.f, 0.f, BulletRedirectMode::None,
			L"KagomeGridBulletGreen", KAGOME_GRID_BULLET_RADIUS, false, -1.f, KAGOME_GRID_BULLET_LIFETIME);
	}

	line.pos += line.step;
	line.remainingBullets--;
	if (line.remainingBullets <= 0)
		line.active = false;
}

void Boss::updateKagomeGrid(float deltaTime)
{
	if (_kagomeDiagonalBackslashPending)
	{
		_kagomeDiagonalBackslashTimer -= deltaTime;
		if (_kagomeDiagonalBackslashTimer <= 0.f)
		{
			startKagomeBackslashLines();
			_kagomeDiagonalBackslashPending = false;
		}
	}

	for (KagomeGridLine& line : _kagomeLines)
	{
		advanceKagomeLine(line, deltaTime);
	}
}

// 워밍업(3초, 최초 1회) -> [단발 -> 1초 대기 -> 부채꼴 3발 -> 2초 대기] 무한 반복.
void Boss::updateKagomeBurst(float deltaTime)
{
	_kagomeBurstTimer += deltaTime;

	switch (_kagomeBurstState)
	{
		case KagomeBurstState::Warmup:
			if (_kagomeBurstTimer >= 3.0f)
			{
				shootKagomeSingleBullet();
				_kagomeBurstState = KagomeBurstState::WaitAfterSingle;
				_kagomeBurstTimer = 0.f;
			}
			break;
		case KagomeBurstState::WaitAfterSingle:
			if (_kagomeBurstTimer >= 1.0f)
			{
				shootKagomeFanBullets();
				_kagomeBurstState = KagomeBurstState::WaitAfterFan;
				_kagomeBurstTimer = 0.f;
			}
			break;
		case KagomeBurstState::WaitAfterFan:
			if (_kagomeBurstTimer >= 2.0f)
			{
				shootKagomeSingleBullet();
				_kagomeBurstState = KagomeBurstState::WaitAfterSingle;
				_kagomeBurstTimer = 0.f;
			}
			break;
	}
}

void Boss::shootKagomeSingleBullet()
{
	if (_isDead)
		return;

	GameScene* scene = Game::GetInstance().GetScene();
	if (scene == nullptr)
		return;

	_attackPoseTimer = 0.3f;

	// 아래 고정이 아니라, 매번 왼쪽/오른쪽 중 무작위로 20도씩 기울여서 쏜다.
	float offsetDeg = (rand() % 2 == 0) ? KAGOME_SINGLE_BULLET_ANGLE : -KAGOME_SINGLE_BULLET_ANGLE;
	Vector dir = Vector(0.f, 1.f).Rotate(DegreeToRadian(offsetDeg));
	scene->CreateBullet(GetPos(), BulletType::Enemy, dir, KAGOME_BIG_BULLET_SPEED,
		false, 180.f, 0.f, 0.f, 0.f, BulletRedirectMode::None, L"KagomeBigBulletYellow", KAGOME_BIG_BULLET_RADIUS);
	trackKagomeBigBullet(GetPos(), dir, KAGOME_BIG_BULLET_SPEED);
}

void Boss::shootKagomeFanBullets()
{
	if (_isDead)
		return;

	GameScene* scene = Game::GetInstance().GetScene();
	if (scene == nullptr)
		return;

	_attackPoseTimer = 0.3f;

	// 아래(0,1) 기준 좌우 60도씩 벌어진 부채꼴 3발(-60/0/+60).
	const float offsets[3] = { -60.f, 0.f, 60.f };
	for (float offsetDeg : offsets)
	{
		Vector dir = Vector(0.f, 1.f).Rotate(DegreeToRadian(offsetDeg));
		scene->CreateBullet(GetPos(), BulletType::Enemy, dir, KAGOME_BIG_BULLET_SPEED,
			false, 180.f, 0.f, 0.f, 0.f, BulletRedirectMode::None, L"KagomeBigBulletYellow", KAGOME_BIG_BULLET_RADIUS);
		trackKagomeBigBullet(GetPos(), dir, KAGOME_BIG_BULLET_SPEED);
	}
}

// 빈 슬롯 하나에 새 그림자를 등록한다. 슬롯이 다 차 있으면(사실상 발생하지 않음) 조용히 무시한다.
void Boss::trackKagomeBigBullet(Vector origin, Vector dir, float speed)
{
	for (KagomeBigBulletShadow& shadow : _kagomeBigBulletShadows)
	{
		if (!shadow.active)
		{
			shadow.pos = origin;
			shadow.dir = dir;
			shadow.speed = speed;
			shadow.active = true;
			return;
		}
	}
}

// 큰 탄 그림자들을 직접 이동시키면서(실제 Bullet과 같은 등속 직선운동이라 계산으로 충분히 따라간다),
// 그 주변 KAGOME_DISRUPT_RADIUS 안에 있는 격자탄(콜라이더 반지름으로 구분)을 큰 탄 반대쪽으로 밀어낸다.
// 밀려난 격자탄은 삭제되지 않고 그대로 날아가다가, 기존 Bullet::Update()의 화면밖 체크에 걸려
// 화면을 벗어나는 순간 자연스럽게 Destroy()되어 풀로 반환된다.
void Boss::updateKagomeDisruption(float deltaTime)
{
	GameScene* scene = Game::GetInstance().GetScene();
	if (scene == nullptr)
		return;

	for (KagomeBigBulletShadow& shadow : _kagomeBigBulletShadows)
	{
		if (!shadow.active)
			continue;

		shadow.pos += shadow.dir * shadow.speed * deltaTime;

		if (shadow.pos.x < -32.f || shadow.pos.x > (float)GWinSizeX + 32.f ||
			shadow.pos.y < -32.f || shadow.pos.y > (float)GWinSizeY + 32.f)
		{
			shadow.active = false;
			continue;
		}

		const vector<Actor*>& bullets = scene->GetRenderList(RenderLayer::Bullet);
		for (Actor* actor : bullets)
		{
			if (actor->GetActorType() != ActorType::EnemyBullet)
				continue;

			Bullet* bullet = static_cast<Bullet*>(actor);
			ColliderCircle* collider = bullet->GetCollider();
			// 반지름으로 "격자탄인지"를 구분한다: 큰 탄(KAGOME_BIG_BULLET_RADIUS=35)은 걸러지고
			// 격자탄(KAGOME_GRID_BULLET_RADIUS=13.5 -> ColliderCircle 내부에서 13으로 절삭)만 대상이 된다.
			if (collider == nullptr || collider->GetRadius() != (int32)KAGOME_GRID_BULLET_RADIUS)
				continue;

			Vector away = bullet->GetPos() - shadow.pos;
			float dist = away.Length();
			if (dist > KAGOME_DISRUPT_RADIUS)
				continue;

			// 큰 탄과 정확히 겹친 경우(거리 0)엔 기준 방향이 없으므로 큰 탄의 진행 방향으로 밀어낸다.
			away = (dist > SMALL_NUMBER) ? away : shadow.dir;
			bullet->SetVelocity(away, KAGOME_SCATTER_SPEED);
		}
	}
}

// ============================================================
// 7페이즈: 사랑의 미로
// Spiral(연속 회전 발사)과 Circle(주기적 링 발사)이 완전히 독립된 타이머로 동시에 돌되,
// 하나의 공유 상태(_loveMazeGapAngle)를 통해 항상 같은 20도 구간을 비우고 쏜다.
// 그 구간을 이동시키는 쪽은 Spiral뿐이고(새 팔이 나갈 때마다 양옆 중 무작위로 한 칸),
// Circle은 그 시점의 값을 그냥 읽기만 한다.
// ============================================================

void Boss::startLoveMazeSpellcard()
{
	_loveMazeActive = true;
	_loveMazeGapAngle = (float)(rand() % 18) * LOVE_MAZE_GAP_WIDTH;	// 0,20,40...340 중 무작위 시작 위치
	_loveMazeSpiralAngle = 0.f;
	_loveMazeCircleAngle = 0.f;

	TimeManager::GetInstance().Remove(_loveMazeSpiralTimerId);
	_loveMazeSpiralTimerId = TimeManager::GetInstance().AddTimer([this]() { shootLoveMazeSpiral(); }, LOVE_MAZE_SPIRAL_INTERVAL, true);

	TimeManager::GetInstance().Remove(_loveMazeCircleTimerId);
	_loveMazeCircleTimerId = TimeManager::GetInstance().AddTimer([this]() { shiftLoveMazeGapAndFireCircle(); }, LOVE_MAZE_CIRCLE_INTERVAL, true);
}

void Boss::stopLoveMazeSpellcard()
{
	_loveMazeActive = false;
	TimeManager::GetInstance().Remove(_loveMazeSpiralTimerId);
	_loveMazeSpiralTimerId = -1;
	TimeManager::GetInstance().Remove(_loveMazeCircleTimerId);
	_loveMazeCircleTimerId = -1;
}

// angleDeg가 [_loveMazeGapAngle, _loveMazeGapAngle + LOVE_MAZE_GAP_WIDTH) 안에 들어가는지.
bool Boss::isAngleInLoveMazeGap(float angleDeg) const
{
	float diff = fmodf(angleDeg - _loveMazeGapAngle + 720.f, 360.f);	// 720 더해서 fmodf 음수 입력을 피한다
	return diff < LOVE_MAZE_GAP_WIDTH;
}

void Boss::shootLoveMazeSpiral()
{
	if (_isDead)
		return;

	GameScene* scene = Game::GetInstance().GetScene();
	if (scene == nullptr)
		return;

	_attackPoseTimer = 0.3f;

	for (int32 i = 0; i < LOVE_MAZE_SPIRAL_ARM_COUNT; ++i)
	{
		float angleDeg = fmodf(_loveMazeSpiralAngle + i * (360.f / LOVE_MAZE_SPIRAL_ARM_COUNT) + 720.f, 360.f);
		if (isAngleInLoveMazeGap(angleDeg))
			continue;

		float radian = DegreeToRadian(angleDeg);
		scene->CreateBullet(GetPos(), BulletType::Enemy, Vector(cosf(radian), sinf(radian)), LOVE_MAZE_SPIRAL_SPEED,
			false, 180.f, 0.f, 0.f, 0.f, BulletRedirectMode::None, L"LoveMazeSpiralGreen", LOVE_MAZE_SPIRAL_BULLET_RADIUS);
	}

	_loveMazeSpiralAngle += LOVE_MAZE_SPIRAL_ROTATION_SPEED;

	// 빈 구간은 여기서 옮기지 않는다: shiftLoveMazeGapAndFireCircle()이 갭 이동과 Circle 발사를
	// 같은 순간에 묶어서 처리한다. Spiral은 그 사이(0.1초 간격)엔 그냥 현재 갭을 그대로 따라 돈다.
}

// LOVE_MAZE_CIRCLE_INTERVAL마다 호출된다. 갭을 옮기는 것과 Circle을 쏘는 것을 같은 순간에 묶어서,
// "갭이 빠지는 시점"과 "Circle 링이 만들어지는 시점"이 항상 정확히 일치하게 한다 — 이게 어긋나면
// Circle 링과 Spiral 궤적의 구멍이 서로 다른 각도가 되어 미로 모양이 안 나온다.
void Boss::shiftLoveMazeGapAndFireCircle()
{
	if (_isDead)
		return;

	GameScene* scene = Game::GetInstance().GetScene();
	if (scene == nullptr)
		return;

	_loveMazeGapAngle += (rand() % 2 == 0) ? LOVE_MAZE_GAP_WIDTH : -LOVE_MAZE_GAP_WIDTH;
	_loveMazeGapAngle = fmodf(_loveMazeGapAngle + 360.f, 360.f);

	_attackPoseTimer = 0.3f;

	for (int32 i = 0; i < LOVE_MAZE_CIRCLE_COUNT; ++i)
	{
		float angleDeg = fmodf(_loveMazeCircleAngle + i * (360.f / LOVE_MAZE_CIRCLE_COUNT) + 720.f, 360.f);
		if (isAngleInLoveMazeGap(angleDeg))
			continue;

		float radian = DegreeToRadian(angleDeg);
		scene->CreateBullet(GetPos(), BulletType::Enemy, Vector(cosf(radian), sinf(radian)), LOVE_MAZE_CIRCLE_SPEED,
			false, 180.f, 0.f, 0.f, 0.f, BulletRedirectMode::None, L"LoveMazeCircleBlue", LOVE_MAZE_CIRCLE_BULLET_RADIUS);
	}

	_loveMazeCircleAngle += LOVE_MAZE_CIRCLE_ROTATION_SPEED;	// 쏠 때마다 살짝 회전시켜 겹겹이 쌓이는 효과
}

// ============================================================
// 8페이즈: 스타보우 브레이크
// 대각선("/" 또는 "\", 6줄) 혹은 십자(가로3줄+세로3줄) 중 하나의 대형을 무작위로 골라, 카고메 격자탄과
// 같은 방식으로 줄마다 한 알씩 순차 스폰한다. 스폰된 탄은 그 자체(Bullet::Update)가 preStopTime 동안
// 위로 떠오르다가, BulletRedirectMode::Down으로 전환되며 속도 0부터 fallAccel(느림/빠름 무작위)로
// 재가속하며 떨어진다 — Boss 쪽은 스폰만 담당하고 상승/낙하는 전적으로 Bullet 내부 상태에 맡긴다.
// 모든 줄의 스폰이 끝나면 STARBOW_WAVE_PAUSE만큼 쉬었다가 다음 웨이브(다른 대형)를 시작한다.
// ============================================================

void Boss::startStarbowBreak()
{
	_starbowActive = true;
	for (StarbowLine& line : _starbowLines)
		line.active = false;

	_starbowState = StarbowState::Spawning;
	_starbowStateTimer = 0.f;
	startStarbowWave();
}

void Boss::stopStarbowBreak()
{
	_starbowActive = false;
	for (StarbowLine& line : _starbowLines)
		line.active = false;
}

// 대형 3종(슬래시/백슬래시/십자) 중 하나를 무작위로 골라 6개 줄을 세팅한다.
void Boss::startStarbowWave()
{
	if (_isDead)
		return;

	StarbowFormationType formation = (StarbowFormationType)(rand() % 3);

	if (formation == StarbowFormationType::DiagonalSlash || formation == StarbowFormationType::DiagonalBackslash)
	{
		// y = x + offset (슬래시) 또는 y = -x + offset (백슬래시) 6줄. 줄마다 오프셋이 STARBOW_DIAGONAL_LINE_OFFSET씩 다르다.
		Vector step = (formation == StarbowFormationType::DiagonalSlash) ? Vector(STARBOW_LINE_SPACING, STARBOW_LINE_SPACING)
																		  : Vector(STARBOW_LINE_SPACING, -STARBOW_LINE_SPACING);
		// step.y가 슬래시는 +, 백슬래시는 -라서 line.pos를 왼쪽 끝(첫 탄)에 그대로 목표 y로 두면
		// 슬래시는 목표 y보다 계속 아래로, 백슬래시는 계속 위로만 벌어져서 두 대형이 서로 다른 높이에
		// 몰리는 것처럼 보인다. 첫 탄이 아니라 "줄 중앙(가운데 탄)"이 목표 y에 오도록 시작 y를 보정한다.
		float centerYOffset = ((float)(STARBOW_BULLETS_PER_LINE - 1) * 0.5f) * step.y;
		for (int32 i = 0; i < 6; ++i)
		{
			float offset = (i - 2.5f) * STARBOW_DIAGONAL_LINE_OFFSET;	// 중앙 기준 좌우로 벌어짐
			StarbowLine& line = _starbowLines[i];
			line.active = true;
			line.pos = Vector(0.f, GWinSizeY * 0.5f + STARBOW_DIAGONAL_Y_SHIFT + offset - centerYOffset);
			line.step = step;
			line.remainingBullets = STARBOW_BULLETS_PER_LINE;
			line.spawnTimer = 0.f;
			line.colorKey = STARBOW_COLOR_KEYS[i % 6];
		}
	}
	else
	{
		// 십자: 가로 3줄(화면 폭 전체) + 세로 3줄(화면 위쪽 600px 구간).
		const float rowYs[3] = { 200.f, 400.f, 600.f };
		const float colXs[3] = { 150.f, 300.f, 450.f };

		for (int32 i = 0; i < 3; ++i)
		{
			StarbowLine& line = _starbowLines[i];
			line.active = true;
			line.pos = Vector(0.f, rowYs[i]);
			line.step = Vector(STARBOW_LINE_SPACING, 0.f);
			line.remainingBullets = STARBOW_BULLETS_PER_LINE;
			line.spawnTimer = 0.f;
			line.colorKey = STARBOW_COLOR_KEYS[i % 6];
		}
		for (int32 i = 0; i < 3; ++i)
		{
			StarbowLine& line = _starbowLines[3 + i];
			line.active = true;
			line.pos = Vector(colXs[i], 0.f);
			line.step = Vector(0.f, STARBOW_LINE_SPACING);
			line.remainingBullets = STARBOW_BULLETS_PER_LINE;
			line.spawnTimer = 0.f;
			line.colorKey = STARBOW_COLOR_KEYS[(3 + i) % 6];
		}
	}
}

// 한 줄을 한 프레임만큼 진행시킨다: 스폰 간격마다 탄 한 알씩 생성(상승->낙하는 Bullet 쪽에 맡김) -> 다음 칸으로 이동.
void Boss::advanceStarbowLine(StarbowLine& line, float deltaTime)
{
	if (!line.active)
		return;

	line.spawnTimer -= deltaTime;
	if (line.spawnTimer > 0.f)
		return;
	line.spawnTimer += STARBOW_BULLET_SPAWN_INTERVAL;

	// 화면 여유범위(-32~+32) 밖이면 생성을 건너뛴다: Bullet::Update()의 화면밖 삭제 체크가
	// 스폰 다음 프레임에 바로 지워버리는 것을 막기 위함(대각선 줄의 꼬리 부분에서 발생).
	if (line.pos.x >= -32.f && line.pos.x <= (float)GWinSizeX + 32.f &&
		line.pos.y >= -32.f && line.pos.y <= (float)GWinSizeY + 32.f)
	{
		float riseHeight = STARBOW_RISE_HEIGHTS[rand() % 3];
		float riseSpeed = riseHeight / STARBOW_RISE_DURATION;
		float fallAccel = (rand() % 2 == 0) ? STARBOW_FALL_ACCEL_SLOW : STARBOW_FALL_ACCEL_FAST;

		// 줄 간격이 딱 맞아떨어지는 격자 느낌을 깨려고, 실제 스폰 x에만 지터를 준다(line.pos 자체는 건드리지 않아
		// 줄의 다음 스폰 위치 계산은 그대로 규칙적으로 진행됨).
		float jitterX = (float)(rand() % (int32)(STARBOW_SPAWN_X_JITTER * 2.f + 1.f)) - STARBOW_SPAWN_X_JITTER;
		Vector spawnPos = Vector(line.pos.x + jitterX, line.pos.y);

		Game::GetInstance().GetScene()->CreateBullet(spawnPos, BulletType::Enemy, Vector(0.f, -1.f), riseSpeed,
			false, 180.f, 0.f, STARBOW_RISE_DURATION, 0.001f, BulletRedirectMode::Down,
			line.colorKey, STARBOW_BULLET_RADIUS, false, -1.f, -1.f, fallAccel);
	}

	line.pos += line.step;
	line.remainingBullets--;
	if (line.remainingBullets <= 0)
		line.active = false;
}

void Boss::updateStarbowBreak(float deltaTime)
{
	if (_starbowState == StarbowState::Waiting)
	{
		_starbowStateTimer -= deltaTime;
		if (_starbowStateTimer <= 0.f)
		{
			_starbowState = StarbowState::Spawning;
			startStarbowWave();
		}
		return;
	}

	bool anyLineActive = false;
	for (StarbowLine& line : _starbowLines)
	{
		advanceStarbowLine(line, deltaTime);
		anyLineActive |= line.active;
	}

	if (!anyLineActive)
	{
		_starbowState = StarbowState::Waiting;
		_starbowStateTimer = STARBOW_WAVE_PAUSE;
	}
}

// ============================================================
// 9페이즈: 과거를 새기는 시계
// 위(270도)/아래(120도, 플레이어 조준) 부채꼴을 같은 타이머(0.5초)로 동시에 쏘고, 날개 4장짜리
// 프로펠러 레이저 2개가 화면 양쪽 끝(x=50/550)에서 출발해 서로 반대 방향으로 회전하며 이동하다가
// 반대쪽 끝에 닿으면 회전까지 완전히 멈추고 2초 쉰 뒤, 방향을 뒤집어 다시 돈다 — 페이즈 끝까지 반복.
// ============================================================

void Boss::startPastClockSpellcard()
{
	_pastClockActive = true;

	GameScene* scene = Game::GetInstance().GetScene();
	if (scene == nullptr)
		return;

	struct PropellerSpawn { Vector pos; float dir; };
	PropellerSpawn spawns[2] =
	{
		// 원래 y차이(500 vs 450 = 50)에서 100 더 벌려서 150차이로(가운데 475 기준 대칭),
		// 이후 30씩 아래로 내렸다가, 바닥 여유 확보를 위해 다시 10씩 위로.
		{ Vector(PAST_CLOCK_PROPELLER_MARGIN, 570.f), 1.f },								// (50,570)에서 오른쪽으로 출발
		{ Vector((float)GWinSizeX - PAST_CLOCK_PROPELLER_MARGIN, 420.f), -1.f },			// (550,420)에서 왼쪽으로 출발
	};

	for (int32 p = 0; p < 2; ++p)
	{
		PastClockPropeller& propeller = _pastClockPropellers[p];
		propeller.pivot = spawns[p].pos;
		propeller.dir = spawns[p].dir;
		propeller.state = PastClockPropellerState::Moving;
		propeller.restTimer = 0.f;

		for (int32 i = 0; i < PAST_CLOCK_PROPELLER_BLADE_COUNT; ++i)
		{
			float angleOffset = i * (360.f / PAST_CLOCK_PROPELLER_BLADE_COUNT);
			Laser* blade = scene->CreateLaser(propeller.pivot, angleOffset, PAST_CLOCK_PROPELLER_BLADE_LENGTH,
				PAST_CLOCK_PROPELLER_SEGMENT_COUNT, PAST_CLOCK_PROPELLER_SEGMENT_RADIUS);
			blade->_bladeTexture = ResourceManager::GetInstance().GetTexture(L"OrbBlue");
			blade->SetAngularSpeed(PAST_CLOCK_PROPELLER_ANGULAR_SPEED);
			// 판정(콜라이더 반지름)은 그대로 두고 그림만 4px 더 두껍게.
			blade->SetBladeThickness((float)(PAST_CLOCK_PROPELLER_SEGMENT_RADIUS * 2) + 4.f);
			// 교차점(pivot) 장식은 날개 4장이 겹쳐 그리지 않도록 첫 번째 날개에만 붙인다.
			if (i == 0)
				blade->_guardTexture = ResourceManager::GetInstance().GetTexture(L"PastClockGuard");
			propeller.blades[i] = blade;
		}
	}

	TimeManager::GetInstance().Remove(_pastClockFanTimerId);
	_pastClockFanTimerId = TimeManager::GetInstance().AddTimer([this]() { shootPastClockFans(); }, PAST_CLOCK_FAN_INTERVAL, true);
}

void Boss::stopPastClockSpellcard()
{
	_pastClockActive = false;
	TimeManager::GetInstance().Remove(_pastClockFanTimerId);
	_pastClockFanTimerId = -1;

	for (PastClockPropeller& propeller : _pastClockPropellers)
	{
		for (Laser*& blade : propeller.blades)
		{
			if (blade != nullptr)
			{
				blade->Destroy();
				blade = nullptr;
			}
		}
	}
}

void Boss::shootPastClockFans()
{
	if (_isDead)
		return;

	GameScene* scene = Game::GetInstance().GetScene();
	if (scene == nullptr)
		return;

	_attackPoseTimer = 0.3f;

	// 위: 270도, 위(0,-1) 방향 중심 — 아래쪽 90도 구간만 비고 나머지는 거의 전방위.
	scene->FireFan(GetPos(), BulletType::Enemy, Vector(0.f, -1.f), PAST_CLOCK_UPPER_FAN_SPREAD,
		PAST_CLOCK_UPPER_FAN_COUNT, PAST_CLOCK_UPPER_FAN_SPEED, L"IllusionFanRed", PAST_CLOCK_FAN_BULLET_RADIUS);

	// 아래: 120도, 그 순간 플레이어 방향 중심으로 조준.
	Player* player = scene->GetPlayer();
	Vector aimDir = (player != nullptr) ? (player->GetPos() - GetPos()) : Vector(0.f, 1.f);
	aimDir.Normalize();
	scene->FireFan(GetPos(), BulletType::Enemy, aimDir, PAST_CLOCK_LOWER_FAN_SPREAD,
		PAST_CLOCK_LOWER_FAN_COUNT, PAST_CLOCK_LOWER_FAN_SPEED, L"IllusionFanRed", PAST_CLOCK_FAN_BULLET_RADIUS);
}

void Boss::updatePastClockPropellers(float deltaTime)
{
	if (_isDead)
		return;

	const float xMin = PAST_CLOCK_PROPELLER_MARGIN;
	const float xMax = (float)GWinSizeX - PAST_CLOCK_PROPELLER_MARGIN;

	for (PastClockPropeller& propeller : _pastClockPropellers)
	{
		if (propeller.state == PastClockPropellerState::Resting)
		{
			propeller.restTimer -= deltaTime;
			if (propeller.restTimer <= 0.f)
			{
				propeller.dir = -propeller.dir;
				propeller.state = PastClockPropellerState::Moving;
				for (Laser* blade : propeller.blades)
				{
					if (blade != nullptr)
						blade->SetAngularSpeed(PAST_CLOCK_PROPELLER_ANGULAR_SPEED);
				}
			}
			continue;
		}

		propeller.pivot.x += propeller.dir * PAST_CLOCK_PROPELLER_MOVE_SPEED * deltaTime;

		bool reachedBound = (propeller.dir > 0.f && propeller.pivot.x >= xMax) ||
							 (propeller.dir < 0.f && propeller.pivot.x <= xMin);
		if (reachedBound)
		{
			propeller.pivot.x = std::clamp(propeller.pivot.x, xMin, xMax);
			propeller.state = PastClockPropellerState::Resting;
			propeller.restTimer = PAST_CLOCK_PROPELLER_REST_DURATION;
			for (Laser* blade : propeller.blades)
			{
				if (blade != nullptr)
					blade->SetAngularSpeed(0.f);
			}
		}

		for (Laser* blade : propeller.blades)
		{
			if (blade != nullptr)
				blade->SetPivot(propeller.pivot);
		}
	}
}

// ============================================================
// 10페이즈: Q.E.D. 495년의 파문
// 원형탄(40발 고정)을 최초 1회는 보스 위치에서, 그 다음부터는 화면 상단의 무작위 위치에서 계속 터뜨린다.
// 각 탄은 좌/우/위 벽에서 딱 1번만 반사(Bullet::_reflectOffWalls)하고, 아래로 빠지면 그대로 삭제된다.
// HP가 QED_HP_TIER_SIZE(100)만큼 깎일 때마다 속도는 +QED_SPEED_STEP, 발사 주기는 -QED_INTERVAL_STEP로
// 계단식으로 빨라진다 — 타이머를 고정 간격으로 걸어두는 대신, 매 프레임 직접 카운트다운하면서 쏘는
// 순간마다 그때그때의 HP로 다음 주기를 다시 계산한다.
// ============================================================

void Boss::startQEDSpellcard()
{
	_qedActive = true;
	_qedFirstBurstFired = false;
	_qedFireTimer = 0.f;	// 0으로 두면 다음 프레임 즉시 첫 발동(보스 위치에서 시작)
}

void Boss::stopQEDSpellcard()
{
	_qedActive = false;
}

void Boss::updateQED(float deltaTime)
{
	if (_isDead)
		return;

	_qedFireTimer -= deltaTime;
	if (_qedFireTimer > 0.f)
		return;

	Vector origin;
	if (!_qedFirstBurstFired)
	{
		origin = GetPos();
		_qedFirstBurstFired = true;
	}
	else
	{
		origin = Vector((float)(rand() % GWinSizeX), QED_TOP_SPAWN_Y);
	}

	shootQEDCircle(origin);

	float tier = std::floor((QED_TIER_BASE_HP - (float)_hp) / QED_HP_TIER_SIZE);
	if (tier < 0.f)
		tier = 0.f;

	float interval = QED_BASE_INTERVAL - tier * QED_INTERVAL_STEP;
	if (interval < QED_MIN_INTERVAL)
		interval = QED_MIN_INTERVAL;

	_qedFireTimer = interval;
}

void Boss::shootQEDCircle(Vector origin)
{
	GameScene* scene = Game::GetInstance().GetScene();
	if (scene == nullptr)
		return;

	_attackPoseTimer = 0.3f;

	float tier = std::floor((QED_TIER_BASE_HP - (float)_hp) / QED_HP_TIER_SIZE);
	if (tier < 0.f)
		tier = 0.f;
	float speed = QED_BASE_SPEED + tier * QED_SPEED_STEP;

	for (int32 i = 0; i < QED_CIRCLE_BULLET_COUNT; ++i)
	{
		float radian = DegreeToRadian(i * (360.f / QED_CIRCLE_BULLET_COUNT));
		Vector dir(cosf(radian), sinf(radian));
		scene->CreateBullet(origin, BulletType::Enemy, dir, speed,
			false, 180.f, 0.f, 0.f, 0.f, BulletRedirectMode::None,
			L"QEDPetalBlue", QED_BULLET_RADIUS, false, -1.f, -1.f, 0.f, true);	// reflectOffWalls=true
	}
}
