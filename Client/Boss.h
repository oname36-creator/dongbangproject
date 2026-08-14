#pragma once

#include "Airplane.h"

enum class BossPatternType
{
	Fan,
	Circle,
	Spiral,
	AimedBurst,	// 조준 라인탄(연사): 방향을 한 번만 고정하고, 그 방향으로 짧은 간격 연사해서 실시간으로 줄이 생기게
	Telegraph,	// 예고 판정: 경고 표시 -> 지연 -> 원형탄 발동
	Grid,
	Cross,
	Random,	// 무작위 방향 난사
	CircleDelayedAimed,		// 시험용: Circle이되 잠시 날아가다 정지 후 플레이어 조준으로 방향 전환
	CircleDelayedRandom,	// 시험용: Circle이되 잠시 날아가다 정지 후 무작위 방향으로 전환
	RandomDelayedRandom,	// 무작위 난사 + 잠시 날아가다 정지 후 다시 무작위 방향으로 전환
	AimedSpread,
	ConvergingBurst,	// 한 방향으로 3발이 각기 다른 속도로 동시에 나갔다가, 목표 속도로 수렴해서 나란히 정렬된다.
	BorderAimedBurst,	// 마법진 6개가 화면 테두리를 따라 돌면서 쉬지 않고 조준탄을 쏜다(실험용).
	Leavatein,
};
struct TimelineStep
{
	float time;
	BossPatternType pattern;
};

enum class LeavateinPhase
{
	Sweep,		// pivot 고정한 채 270도 회전
	Pause,		// Sweep/Slide가 끝난 직후, 칼날 없이 잠깐 멈춰서 화면의 탄환이 빠질 시간을 준다
	Reposition,	// 칼날 없이 다음 액션의 시작 위치로 이동
	Slide,		// 칼날 아래로 고정한 채 반대쪽 화면 끝까지 이동
};

// 레바테인이 반복하는 4가지 동작. 직전에 한 것과 "완전히 같은" 액션만 아니면
// 나머지 3개 중 랜덤으로 다음 액션이 정해진다 (Boss::pickNextLeavateinAction 참고).
enum class LeavateinAction
{
	SweepLeftStart,		// 가운데에서 왼쪽 시작 스윕
	SweepRightStart,	// 가운데에서 오른쪽 시작 스윕
	SlideToLeft,		// 오른쪽 끝에서 왼쪽 끝으로 슬라이드
	SlideToRight,		// 왼쪽 끝에서 오른쪽 끝으로 슬라이드
};

struct BossPhase
{
	vector<TimelineStep> timeline;
	float cycleLength;
	int32 hpThreshold;
};

enum class BossAnimState
{
	Idle,
	Move,
	Attack
};
class Laser;
class Boss : public Airplane
{
	using Super = Airplane;

public:
	void Init(Vector pos, wstring key, vector<BossPhase> phases, int32 maxHp = 100,
			  float bulletCountMul = 1.0f, float bulletSpeedMul = 1.0f,
			  int32 spiralArmCount = 2, float spiralRotationSpeed = 10.f, float spiralInterval = 0.1f,
			  int32 randomShotCount = 8, float randomAccel = 100.f, bool randomAlternateAccel = true,
			  float fanAngleSpread = 60.f, int32 fanShotCount = 5,
			  int32 randomDelayedShotCount = 12, float randomDelayedSpeed = 250.f,
			  float randomDelayedPreStop = 1.0f, float randomDelayedDelay = 0.8f,
			int32 spreadShotCount = 20, float spreadAngle = 30.f,
			float spreadMinSpeed = 200.f, float spreadMaxSpeed = 400.f,
			int32 circleShotCount = 12, wstring circleTextureKey = L"",
			wstring burstTextureKey = L"", float circleColliderSize = -1.f,
			int32 decorPhaseIndex = -1,
			wstring burstTextureKeyAlt = L"", int32 burstTextureAltPhaseIndex = -1, float burstColliderSizeAlt = -1.f,
			wstring randomTextureKey = L"", wstring spiralTextureKey = L"", wstring fanTextureKey = L"", wstring randomDelayedTextureKey = L"",
			bool bulletsFaceDirection = false, float faceDirectionColliderSize = -1.f,
			wstring circleDelayedAimedTextureKey = L"", wstring circleDelayedRandomTextureKey = L"",
			int32 fixedPosPhaseIndex = -1, float circleRotationSpeed = 0.f,
			float convergingSpeed = 300.f, float convergingSpeedSpread = 150.f, float convergingInterval = 0.6f,
			float convergingAngleSpread = 10.f);
	virtual void Destroy() override;

	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;
	virtual void OnEnter(Actor* other) override;
	virtual RenderLayer GetRenderLayer() override { return RenderLayer::Boss; }
	virtual ActorType GetActorType() override { return ActorType::Boss; }

	int32 GetHp() const { return _hp;}
	int32 GetMaxHp() const{ return _maxHp; }
	int32 GetCurPhaseIndex() const {return _curPhaseIndex;}
	int32 GetPhaseCount() const {return (int32)_phases.size();}
	
	

private:
	// 기존 탄 삭제 -> 연출 -> 다음 페이즈로 전환
	void transitionToNextPhase();
	void shootBullet(BossPatternType pattern);
	void shootSpiralBullet();
	void shootTelegraphBullet();
	void shootAimedBurst();
	void setAnimState(BossAnimState state);
	void shootSpreadBullet();
	void shootCrossBullet();
	void shootConvergingBurst();
	void shootBorderAimedBurst();
	void initBorderMarkers();
	void updateBorderMarkers(float deltaTime);
	Vector getBorderCornerPos(int32 cornerIndex) const;
	Vector getBorderMarkerPos(int32 markerIndex) const;

	void updateLeavatein(float deltaTime);
	void pickNextLeavateinAction();
	void startLeavateinPause();
	void startLeavateinReposition();
	void beginLeavateinAction(LeavateinAction action);
	Vector getLeavateinLaunchPos(LeavateinAction action) const;
	void shootLeavateinBullets();

private:
	int32 _hp = 0;
	int32 _maxHp = 100;

	vector<BossPhase> _phases;
	int32 _curPhaseIndex =0;

	int32 _spiralShootTimerId = -1;
	int32 _telegraphTimerId = -1;
	float _spiralAngle = 0.f;

	// Cross도 Spiral처럼 페이즈 내내 일정 간격으로 계속 쏘는 연속 발사로 처리한다.
	int32 _crossShootTimerId = -1;

	int32 _spreadShootTimerId = -1;
	int32 _spreadShotsRemaining = 0;
	Vector _spreadBaseDir;
	int32 _spreadShotCount =20;
	float _spreadAngle = 30.f;
	float _spreadMinSpeed = 200.f;
	float _spreadMaxSpeed = 400.f;

	// 보스별로 Spiral 팔 개수/회전 속도/발사 간격을 조절하기 위한 값. 기본값은 기존 동작과 동일.
	int32 _spiralArmCount = 2;
	float _spiralRotationSpeed = 10.f;
	float _spiralInterval = 0.1f;

	// AimedBurst용: 방향을 한 번만 고정해두고, 짧은 간격으로 남은 발수만큼 연사한다.
	int32 _burstShootTimerId = -1;
	int32 _burstShotsRemaining = 0;
	int32 _burstTotalShots = 0;	// 첫 발부터 몇 번째 발인지 계산해서 점점 빨라지는 속도를 주기 위함
	Vector _burstDir;
	wstring _burstTextureKey = L"";	// 보스별로 AimedBurst 탄 텍스처를 조절하기 위한 값.

	// 특정 페이즈에서만 AimedBurst 텍스처/콜라이더를 다르게 쓰기 위한 값. phaseIndex가 -1이거나
	// 텍스처가 비어있으면 위 _burstTextureKey(기본값)를 그대로 쓴다.
	wstring _burstTextureKeyAlt = L"";
	int32 _burstTextureAltPhaseIndex = -1;
	float _burstColliderSizeAlt = -1.f;

	float _phaseElapsedTime=0.f;
	int32 _timelineIndex =0;

	Vector _moveTargetPos;
	int32 _moveTimerId = -1;
	float _moveSpeed = 100.f;

	bool _isDead = false;

	// 보스별로 Fan/Circle/Spiral 탄막의 탄수/속도를 조절하기 위한 배율.
	float _bulletCountMul = 1.0f;
	float _bulletSpeedMul = 1.0f;

	// idle/move/attack 3종 애니메이션. Init()에 넘어온 key + "Idle"/"Move"/"Attack" 텍스처를 사용한다.
	class SpriteAnimRenderer* _animRenderer = nullptr;
	wstring _baseKey;
	BossAnimState _animState = BossAnimState::Idle;
	float _attackPoseTimer = 0.f;
	bool _randomAccelToggle = false;

	// 보스별로 Random(난사) 탄수/가속값/가감속 번갈아여부를 조절하기 위한 값.
	int32 _randomShotCount = 8;
	float _randomAccel = 100.f;
	bool _randomAlternateAccel = true;
	wstring _randomTextureKey = L"";	// 보스별로 Random 탄 텍스처를 조절하기 위한 값.

	// 보스별로 Fan 탄막의 부채꼴 각도/탄수를 조절하기 위한 값.
	float _fanAngleSpread = 60.f;
	int32 _fanShotCount = 5;
	wstring _fanTextureKey = L"";	// 보스별로 Fan 탄 텍스처를 조절하기 위한 값.

	wstring _spiralTextureKey = L"";	// 보스별로 Spiral 탄 텍스처를 조절하기 위한 값.

	// 보스별로 Circle 탄막의 탄수/텍스처/콜라이더 크기를 조절하기 위한 값.
	int32 _circleShotCount = 12;
	wstring _circleTextureKey = L"";
	float _circleColliderSize = -1.f;
	// Circle을 연속으로 쏠 때마다 이만큼씩 회전시켜서, 겹겹이 쌓이는 꽃잎 모양을 만든다. 0이면 항상 같은 각도(기존 동작).
	float _circleRotationSpeed = 0.f;
	float _circleAngle = 0.f;

	// RandomDelayedRandom(난사 + 정지 후 재무작위)용 값.
	int32 _randomDelayedShotCount = 12;
	float _randomDelayedSpeed = 250.f;
	float _randomDelayedPreStop = 1.0f;
	float _randomDelayedDelay = 0.8f;
	wstring _randomDelayedTextureKey = L"";	// 보스별로 RandomDelayedRandom 탄 텍스처를 조절하기 위한 값.

	// true면 위 텍스처들(Fan/Random/Spiral/RandomDelayedRandom/AimedBurst)을 dir 방향에 맞춰 16방향 중
	// 가장 가까운 쪽으로 회전시켜서 그린다(단검처럼 방향성 있는 텍스처용). 미리 회전된 9장(0~180도)을
	// 두고 나머지는 좌우 반전으로 대체하는 방식이라, 텍스처 키가 "이름_각도" 형태로 등록돼 있어야 한다.
	bool _bulletsFaceDirection = false;
	// 위 방향 회전 텍스처들의 콜라이더 반지름(회전 스프라이트 크기에 맞춰 보스별로 다르게 준다. -1이면 auto).
	float _faceDirectionColliderSize = -1.f;

	// CircleDelayedAimed/CircleDelayedRandom 전용 텍스처. (일반 Circle은 _circleTextureKey를 그대로 씀)
	wstring _circleDelayedAimedTextureKey = L"";
	wstring _circleDelayedRandomTextureKey = L"";

	// 특정 페이즈에서만 보스 뒤에 연출용 마법진을 그리기 위한 값. -1이면 안 그림.
	int32 _decorPhaseIndex = -1;

	// 특정 페이즈에서만 보스를 화면 중앙(GWinSizeX*0.5, 150)에 고정시키기 위한 값. -1이면 평소처럼 랜덤 이동.
	int32 _fixedPosPhaseIndex = -1;

	// ConvergingBurst: Spiral/Cross처럼 페이즈 내내 일정 간격으로 계속 쏘는 연속 발사로 처리한다.
	// 3발이 각각 (목표속도-스프레드) / 목표속도 / (목표속도+스프레드)로 시작해서 전부 목표속도로 수렴한다.
	int32 _convergingBurstTimerId = -1;
	float _convergingSpeed = 300.f;
	float _convergingSpeedSpread = 150.f;
	float _convergingInterval = 0.6f;
	float _convergingAngleSpread = 10.f;	// 3발이 같은 방향이 아니라 이 각도만큼씩 벌어져서 나간다.

	// BorderAimedBurst(실험용): 마법진 6개. 그룹A(1,2,3)는 (0,0)에서, 그룹B(4,5,6)는 대각선 반대
	// 코너에서 시작. 1/4번은 플레이어를 조준하고 나머지는 화면 중앙을 조준한다.
	// 각자 완전히 독립적으로 코너에 도착할 때마다 시계/반시계 방향을 랜덤으로 고르며 테두리를 따라 이동
	// (인접 코너는 항상 X축 또는 Y축 중 한쪽만 다르므로, 결과적으로 이동 축도 랜덤이 된다).
	// 3번/6번만 시작 시점 타이머를 0.2초 앞당겨서 리더와 살짝 어긋나 보이게 한다(진짜로 경로를 따라가진 않음).
	struct BorderMarker
	{
		int32 fromCorner = 0;
		int32 toCorner = 1;
		float segmentTime = 0.f;
		bool aimAtPlayer = false;
	};
	BorderMarker _borderMarkers[6];
	int32 _borderBurstTimerId = -1;
	float _borderBurstInterval = 0.3f;
	float _borderBulletSpeed = 100.f;
	float _borderGlideDuration = 1.5f;
	float _borderPauseDuration = 1.0f;

	// Reposition 상태는 칼날이 없는(_leavateinLaser == nullptr) 구간이라, "패턴이 활성 중인지"는
	// 칼날 존재 여부가 아니라 이 플래그로 따로 추적해야 한다.
	bool _leavateinActive = false;
	Laser* _leavateinLaser = nullptr;
	LeavateinPhase _leavateinPhase = LeavateinPhase::Sweep;
	float _leavateinStateTimer = 0.f;
	int32 _leavateinBulletTimerId = -1;

	LeavateinAction _leavateinLastAction = LeavateinAction::SweepLeftStart;	// 방금 끝낸 액션 (다음 액션 뽑을 때 이것만 제외)
	LeavateinAction _leavateinPendingAction = LeavateinAction::SweepLeftStart;	// Reposition이 끝나면 시작할 액션
	Vector _leavateinMoveFrom;	// Reposition/Slide 이동 시작 위치
	Vector _leavateinMoveTo;	// Reposition/Slide 이동 목표 위치
};
