#pragma once
#include "Actor.h"

class Background : public Actor
{
	using Super = Actor;
public:
	void Init(std::wstring textureKey, float moveSpeed);
	void ChangeTexture(std::wstring textureKey);
	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;
	virtual RenderLayer GetRenderLayer() override { return RenderLayer::Background; }
	virtual ActorType GetActorType() override { return ActorType::Background; }
private:
	//class Texture* _texture = nullptr;
	class ImageRenderer* _renderer = nullptr;

	float _moveSpeed = 300;
	int32 _textureHeight = 0;
	// 두개의 Map Texture를 로테이션한다
	Vector _pos2;
};

