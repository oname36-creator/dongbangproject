#pragma once

#include "Airplane.h"

enum class EnemyType
{
	Zigzag,
	Fan,
	Circle,
	Aimed
};

class Enemy : public Airplane
{
	using Super = Airplane;

public:
	void Init(Vector pos, wstring key);
	virtual void Destroy() override;

	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;
	virtual void OnEnter(Actor* other) override;
	virtual RenderLayer GetRenderLayer() override { return RenderLayer::Enemy; }
	virtual ActorType GetActorType() override { return ActorType::Enemy; }

private:
	void shootBullet();

private:
	class SpriteAnimRenderer* _animRenderer = nullptr;

	int32 _hp = 2;
	EnemyType _type;

	float _moveSpeedX = 50;
	float _moveSpeedY = 50;

	// 원처럼 그래프 그리면서 내려오게 하려고
	float _sumRadian = 0;
	float _turnSpeed = 2;

	bool _isDead = false;

	// 주기적으로 총알 발사하는 Timer
	int32 _shootTimerId = -1;
};
