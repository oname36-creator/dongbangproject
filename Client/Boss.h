#pragma once

#include "Airplane.h"

// TODO(2주차 Day4): 페이즈별로 필요한 정보를 채워라.
//  예: 이 페이즈에서 쏠 패턴(기존 FireFan/FireCircle/FireSpiral 등 재사용),
//      다음 페이즈로 넘어가는 hp 임계값, 페이즈 이름(체력바 표시용) 등.
// TODO(2주차 Day5): 최종(3번째) 페이즈 - Spiral 추가
//  1. 여기에 Spiral 값을 추가해라.
//  2. Boss::Init()의 _phases 초기화 목록에 {BossPatternType::Spiral, ...} 을 3번째로 추가해라.
//     (마지막 페이즈라 hpThreshold는 transitionToNextPhase()가 다음 페이즈로 안 넘어가므로 의미 없다 - Boss::OnEnter()의
//      "_curPhaseIndex < _phases.size()-1" 조건 참고)
//  3. FireSpiral(GameScene.h)은 회전각을 프레임마다 누적해야 동작한다(& rotationAngle 참조 인자).
//     즉 Boss에 float _spiralAngle = 0.f; 같은 멤버가 있어야 하고, shootBullet()에서 매 호출마다
//     그 멤버를 넘겨야 다음 발사 때 이어진다.
//  4. shootBullet()의 switch문에 case BossPatternType::Spiral: 을 추가하고
//     Game::GetInstance().GetScene()->FireSpiral(GetPos(), BulletType::Enemy, count, speed, _spiralAngle, rotationSpeed) 호출.
enum class BossPatternType
{
	Fan,
	Circle,
	Spiral,
	Homing,		// 유도탄
	Telegraph,	// 예고 판정: 경고 표시 -> 지연 -> 원형탄 발동
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
	void shootBullet();
	void shootSpiralBullet();
	void shootTelegraphBullet();
private:
	int32 _hp = 0;
	int32 _maxHp = 100;

	vector<BossPhase> _phases;
	int32 _curPhaseIndex =0;
	int32 _shootTimerId = -1;
	int32 _spiralShootTimerId = -1;
	int32 _telegraphTimerId = -1;
	float _spiralAngle = 0.f;

	Vector _moveTargetPos;
	int32 _moveTimerId = -1;
	float _moveSpeed = 100.f;
};
