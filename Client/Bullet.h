#pragma once
#include "Actor.h"

class Bullet : public Actor // (Actor=GameObject)
{
	using Super = Actor;
public:
	void Init(BulletType type, Vector dir, float speed, bool isHoming = false, float turnSpeed = 180.f, float accel = 0.f,
			  float preStopTime = 0.f, float launchDelay = 0.f, BulletRedirectMode redirectMode = BulletRedirectMode::None);
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
	bool _isHoming = false;
	float _turnSpeed = 180.f;
	float _accel = 0.f;

	float _preStopTime = 0.f;
	float _launchDelay = 0.f;
	BulletRedirectMode _redirectMode = BulletRedirectMode::None;
};