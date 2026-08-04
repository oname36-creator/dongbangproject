#pragma once
#include "Airplane.h"

class Player : public Airplane
{
	using Super = Airplane;
public:
	void Init();
	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;
	virtual void OnEnter(Actor* other) override;
	virtual RenderLayer GetRenderLayer() override { return RenderLayer::Player; }
	virtual ActorType GetActorType() override { return ActorType::Player; }
	int32 GetLives() const { return _lives; }
	int32 GetBoom() const {return _boom;}
	int32 GetMaxHp() const { return 100; }
private:
	void move(float x, float y);
	void takeDamage();

private:
	int32 _attack = 1;	// 공격력
	float _moveSpeed = 300.f;
	float _speed = 0.f;
	int32 _boom = 2;

	float _invincibleTime = 0.f;

	float _fireCooldown = 0.f;
	float _fireInterval =0.1f;

	// 보조 공격(유도탄): 가장 가까운 적을 자동으로 쫓아간다
	float _subFireCooldown = 0.f;
	float _subFireInterval = 0.5f;

	bool _debugInvincible = false;

	int32 _powerLevel = 0; 
	int32 _lives = 3;
};


