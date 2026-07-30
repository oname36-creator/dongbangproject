#pragma once

#include "Airplane.h"

// TODO(2주차 Day4): 페이즈별로 필요한 정보를 채워라.
//  예: 이 페이즈에서 쏠 패턴(기존 FireFan/FireCircle/FireSpiral 등 재사용),
//      다음 페이즈로 넘어가는 hp 임계값, 페이즈 이름(체력바 표시용) 등.
enum class BossPatternType
{
	Fan,
	Circle
};

struct BossPhase
{
	BossPatternType pattern;
	int32 hpThreshold;
};

class Boss : public Airplane
{
	using Super = Airplane;

public:
	void Init(Vector pos, wstring key);
	virtual void Destroy() override;

	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;
	virtual void OnEnter(Actor* other) override;
	virtual RenderLayer GetRenderLayer() override { return RenderLayer::Boss; }
	virtual ActorType GetActorType() override { return ActorType::Boss; }
	
	

private:
	// 기존 탄 삭제 -> 연출 -> 다음 페이즈로 전환
	void transitionToNextPhase();
	void shootBullet();

private:
	int32 _hp = 0;

	// TODO(2주차 Day4): 페이즈 목록(vector<BossPhase> 등)과 현재 페이즈 인덱스
	vector<BossPhase> _phases;
	int32 _curPhaseIndex =0;
	int32 _shootTimerId = -1;

	Vector _moveTargetPos;
	int32 _moveTimerId = -1;
	float _moveSpeed = 100.f;
};
