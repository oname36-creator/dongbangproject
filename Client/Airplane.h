#pragma once
#include "Actor.h"

class Airplane : public Actor
{
	using Super = Actor;
public:
	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;
	virtual class ColliderCircle* GetCollider() override { return _collider; }

	int32 GetWidth();
	int32 GetHeight();

protected:
	void loadTexture(wstring key);

protected:
	//class Texture* _texture = nullptr;
	// 
	// Actor가 관리하는 Component구조로 변경 
	class ImageRenderer* _renderer = nullptr;
	class ColliderCircle* _collider = nullptr;
};

