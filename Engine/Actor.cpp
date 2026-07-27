#include "pch.h"
#include "Actor.h"
#include "Game.h"
#include "Scene.h"
#include "Component.h"

Actor::~Actor()
{
	// new Component 들.. 메모리 해제.
	for (auto component : _components)
	{
		delete component;
	}
	_components.clear();
}

// Actor 파괴(삭제) 싶으면, 무조건 Scene에 예약을 걸어서 처리한다.
void Actor::Destroy()
{
	// 1번 방식
	//_pendingKill = true; // 삭제 예약 상태 flag 추가해도 된다.
	
	// 2번 방식
	Game::GetInstance().GetScene()->DeleteActor(this);
}

void Actor::Update(float deltaTime)
{
	for (auto component : _components)
	{
		component->Update(deltaTime);
	}
}

void Actor::Render(HDC hdc)
{
	for (auto component : _components)
	{
		component->Render(hdc, GetPos());
	}
}
