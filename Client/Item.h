#pragma once
#include "Actor.h"

enum class ItemKind
{
	Power,
	Score
};
class Item : public Actor
{
	using Super = Actor;
public:
	void Init(Vector pos, ItemKind kind, int32 powerValue = 1, bool burst = false, bool autoCollect = false);
	ItemKind GetKind() const { return _kind; };
	int32 GetPowerValue() const { return _powerValue; }
	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;

	virtual RenderLayer GetRenderLayer() override { return RenderLayer::Item; }
	virtual ActorType GetActorType() override { return ActorType::Item; }
	virtual class ColliderCircle* GetCollider() override { return _collider; }

private:
	class ImageRenderer* _renderer = nullptr;
	class ColliderCircle* _collider = nullptr;
	float _fallSpeed = 60.f;
	ItemKind _kind = ItemKind::Power;
	int32 _powerValue = 1;

	// 피격 스캐터용: 위로 튀어올랐다가 중력에 의해 서서히 느려지며 다시 떨어지는 연출
	float _velocityX = 0.f;
	float _velocityY = 0.f;
	float _gravity = 600.f;

	// 폭탄 사용 시 탄환 -> 점수 아이템 전환용: 낙하 대신 플레이어를 향해 자동으로 날아간다.
	bool _autoCollect = false;
	float _collectSpeed = 500.f;
};
