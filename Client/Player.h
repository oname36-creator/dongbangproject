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

	// TODO(1주차 Day3~4): HP 체계를 '잔기/폭탄' 체계로 교체할 것
	//  현재: _hp = 100, 피격당 -10 → 10대를 맞아야 죽는다. 탄막 슈팅의 규칙이 아니다.
	//  목표(기획서 5장): 피격 1회 = 잔기 1 감소 / 초기 잔기 3 / 초기 폭탄 2 / 피격 후 무적 1.5~2초
	//  할 일:
	//   - _hp 를 _life(=3), _bomb(=2) 로 교체하고 _invincibleTime(float) 을 추가
	//   - 잔기가 0이 되면 게임 오버 (GameScene 상태머신으로 알려야 한다)
	//  힌트: 무적 시간은 Update(deltaTime)에서 직접 빼주는 게 가장 단순하다.
	//        TimeManager::AddTimer는 '반복 이벤트'용이라 여기엔 과하다.
	//  주의: GetHp()/GetMaxHp()를 UIManager가 쓰고 있다. 지우면 컴파일 에러가 나니 같이 고칠 것.
	int32 _lives = 3;
};


