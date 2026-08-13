#pragma once
#include "Actor.h"

class Bullet : public Actor // (Actor=GameObject)
{
	using Super = Actor;
public:
	void Init(BulletType type, Vector dir, float speed, bool isHoming = false, float turnSpeed = 180.f, float accel = 0.f,
			  float preStopTime = 0.f, float launchDelay = 0.f, BulletRedirectMode redirectMode = BulletRedirectMode::None,
			  wstring customTextureKey = L"", float colliderSizeOverride = -1.f, bool faceDirection = false, float targetSpeed = -1.f);
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
	// faceDirection이 켜져있을 때, _dir 기준으로 16방향 중 가까운 회전 텍스처를 골라서 적용한다.
	// 방향 재조준(redirect) 시 다시 호출해서 렌더링도 같이 돌아가게 한다.
	void updateFaceDirectionTexture();

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
	float _targetSpeed = -1.f;	// -1이면 기존처럼 무제한 가속/감속. 0 이상이면 이 속도에 도달한 순간 가속을 끄고 고정한다.

	float _preStopTime = 0.f;
	float _launchDelay = 0.f;
	BulletRedirectMode _redirectMode = BulletRedirectMode::None;

	bool _faceDirection = false;
	wstring _baseTextureKey;	// faceDirection용 회전 접미사가 붙기 전의 원본 텍스처 키
};