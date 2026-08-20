#pragma once
#include "Actor.h"

// Laser의 칼날을 구성하는 개별 원 콜라이더 세그먼트.
// 렌더링은 하지 않는다 (칼날 도형 전체는 Laser가 한 번에 그린다). 판정 전용 액터.
class LaserSegment : public Actor
{
	using Super = Actor;
public:
	void Init(int radius);
	virtual void Render(HDC hdc) override {}

	virtual RenderLayer GetRenderLayer() override { return RenderLayer::Enemy; }
	virtual ActorType GetActorType() override { return ActorType::EnemyLaser; }
	virtual class ColliderCircle* GetCollider() override { return _collider; }

private:
	class ColliderCircle* _collider = nullptr;
};
