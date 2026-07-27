#pragma once
#include "Actor.h"

class Effect : public Actor
{
	using Super = Actor;
public:
	void Init(wstring textureKey);
	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;
	virtual RenderLayer GetRenderLayer() override { return RenderLayer::Effect; }
	virtual ActorType GetActorType() override { return ActorType::Effect; }
private:
	class SpriteAnimRenderer* _renderer = nullptr;
};

