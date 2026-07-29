#pragma once
#include "Actor.h"

class Bullet : public Actor // (Actor=GameObject)
{
	using Super = Actor;
public:
	void Init(BulletType type, Vector dir, float speed = 500.f);
	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;

	virtual RenderLayer GetRenderLayer() override { return RenderLayer::Bullet; }
	virtual ActorType GetActorType() override 
	{
		// 본인이 어떤 총알인지 판단
		if(_type == BulletType::Enemy)
			return ActorType::EnemyBullet; 
		
		return ActorType::PlayerBullet;
	}


	virtual class ColliderCircle* GetCollider() override { return _collider; }

	BulletType GetBulletType() const { return _type; }

private:
	//class Texture* _texture = nullptr;
	//class ImageRenderer* _renderer = nullptr;
	//class SpriteAnimRenderer* _renderer = nullptr;
	class ColliderCircle* _collider = nullptr;

	Vector _dir;		// 발사 방향
	float _moveSpeed = 500.f;	// 발사 속도
	BulletType _type;
};

// 나중에 아이템이 추가되어도 크게 코드를 수정해야할 일이 없다.
class Item : public Actor
{
private:
	class Texture* _texture = nullptr;
	class ColliderCircle* _collider = nullptr;
};