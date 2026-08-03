#pragma once

#include "Airplane.h"

enum class BossPatternType
{
	Fan,
	Circle,
	Spiral,
	AimedBurst,	// 조준 라인탄(연사): 방향을 한 번만 고정하고, 그 방향으로 짧은 간격 연사해서 실시간으로 줄이 생기게
	Telegraph,	// 예고 판정: 경고 표시 -> 지연 -> 원형탄 발동
	Grid
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

class Boss : public Airplane
{
	using Super = Airplane;

public:
	void Init(Vector pos, wstring key, vector<BossPhase> phases, int32 maxHp = 100);
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
	Vector _burstDir;

	float _phaseElapsedTime=0.f;
	int32 _timelineIndex =0;

	Vector _moveTargetPos;
	int32 _moveTimerId = -1;
	float _moveSpeed = 100.f;
};
