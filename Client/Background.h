#pragma once
#include "Actor.h"

class Background : public Actor
{
	using Super = Actor;
public:
	void Init(std::wstring textureKey, float moveSpeed, float moveSpeedX = 0.f);
	void ChangeTexture(std::wstring textureKey, float moveSpeedX = 0.f, float moveSpeed = -1.f);
	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;
	virtual RenderLayer GetRenderLayer() override { return RenderLayer::Background; }
	virtual ActorType GetActorType() override { return ActorType::Background; }
private:
	//class Texture* _texture = nullptr;
	class ImageRenderer* _renderer = nullptr;

	float _moveSpeed = 300;
	float _moveSpeedX = 0.f;
	int32 _textureHeight = 0;
	int32 _textureWidth = 0;
	float _scrollX = 0.f;
	// 두개의 Map Texture를 로테이션한다
	Vector _pos2;
};

