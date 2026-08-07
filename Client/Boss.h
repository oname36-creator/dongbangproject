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
};
struct TimelineStep
{
	float time;
	BossPatternType pattern;
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

class Boss : public Airplane
{
	using Super = Airplane;

public:
	void Init(Vector pos, wstring key, vector<BossPhase> phases, int32 maxHp = 100,
			  float bulletCountMul = 1.0f, float bulletSpeedMul = 1.0f);
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

private:
	int32 _hp = 0;
	int32 _maxHp = 100;

	vector<BossPhase> _phases;
	int32 _curPhaseIndex =0;

	int32 _spiralShootTimerId = -1;
	int32 _telegraphTimerId = -1;
	float _spiralAngle = 0.f;

	// AimedBurst용: 방향을 한 번만 고정해두고, 짧은 간격으로 남은 발수만큼 연사한다.
	int32 _burstShootTimerId = -1;
	int32 _burstShotsRemaining = 0;
	int32 _burstTotalShots = 0;	// 첫 발부터 몇 번째 발인지 계산해서 점점 빨라지는 속도를 주기 위함
	Vector _burstDir;

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
};
