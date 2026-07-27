#pragma once
#include "Actor.h"

class WorldBG : public Actor
{
	using Super = Actor;
public:
	void Init();
	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;
	virtual RenderLayer GetRenderLayer() override { return RenderLayer::Background; }
	virtual ActorType GetActorType() override { return ActorType::Background; }
	
	Vector GetMapSize();

private:
	//class Texture* _texture = nullptr;
	class ImageRenderer* _renderer = nullptr;
};

 