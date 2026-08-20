#pragma once

#include "Airplane.h"

// 엑스트라 보스 5페이즈 "분신" 전용 경량 액터.
// Boss와 달리 페이즈/타임라인 개념이 없다: 화면 위에서 목표 위치까지 내려오는 입장 연출 후,
// 스폰 지점 주변 작은 범위에서만 배회하며 Circle+Fan을 주기적으로(서로 시간차를 두고) 쏘기만 한다.
// 이 액터의 HP는 진짜 보스 HP와 완전히 분리되어 있고, 죽어도 점수/페이즈 전환에 영향을 주지 않는다.
class BossIllusion : public Airplane
{
	using Super = Airplane;

public:
	// pos: 입장 연출이 끝난 뒤 자리잡을 최종 위치(배회 중심점이기도 함). 실제 스폰은 이 위치의 화면 위쪽에서 시작해 내려온다.
	// key: 본체와 동일한 스프라이트를 써야 시각적으로 구분이 안 된다 (예: L"ExtraBoss").
	// shootOffset: 착지 후 첫 발사까지 몇 초 걸릴지 (본체/다른 분신과 캐스케이드를 만들기 위함).
	// fanTextureKey: 이 분신의 Fan 탄 색상. 분신마다 다르게 줘서 본체/다른 분신과 구분되게 한다(Circle 탄은 공용 링 텍스처 고정).
	void Init(Vector pos, wstring key, int32 hp, float shootOffset, wstring fanTextureKey);
	virtual void Destroy() override;

	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;
	virtual void OnEnter(Actor* other) override;
	virtual RenderLayer GetRenderLayer() override { return RenderLayer::Boss; }
	virtual ActorType GetActorType() override { return ActorType::Boss; }

private:
	// 스폰 위치(_wanderCenter) 기준 아주 작은 반경 안에서 배회할 다음 목표점을 재선정.
	void pickWanderTarget();

	// Circle+Fan 발사. GameScene::FireCircle/FireFan 재사용.
	void shootBullet();

	// 입장 연출(하강)이 끝난 시점에 배회/발사 타이머를 시작.
	void beginActiveBehavior();

	// 본체(Boss::setAnimState)와 같은 방식: 이동 중이면 Move, 멈춰있으면 Idle.
	void setAnimState(bool isMoving);

private:
	class SpriteAnimRenderer* _animRenderer = nullptr;
	wstring _baseKey;
	bool _isMoving = false;
	wstring _fanTextureKey;

	int32 _hp = 30;
	bool _isDead = false;

	// 화면 위(_entryStart)에서 목표 위치(_entryTarget)까지 일직선으로 내려오는 입장 연출.
	bool _isEntering = true;
	Vector _entryStart;
	Vector _entryTarget;
	float _entryT = 0.f;
	static constexpr float ENTRY_DURATION = 0.6f;

	Vector _wanderCenter;
	Vector _wanderTargetPos;
	float _wanderRadius = 60.f;
	float _moveSpeed = 100.f;	// 다른 보스들과 동일한 이동 속도
	int32 _wanderTimerId = -1;

	int32 _shootTimerId = -1;
	float _shootInterval = 1.0f;
	float _shootOffset = 0.f;
};
