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
	void Init(Vector pos, ItemKind kind);
	ItemKind GetKind() const { return _kind; };
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
};
