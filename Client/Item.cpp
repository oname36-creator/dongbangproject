#include "pch.h"
#include "Item.h"
#include "ImageRenderer.h"
#include "ColliderCircle.h"

void Item::Init(Vector pos)
{
	SetPos(pos);
	ImageRenderer* renderer = GetComponent<ImageRenderer>();
	if(renderer == nullptr)
		renderer = AddComponent<ImageRenderer>();
	
	renderer->Init(L"PowerItem");

	// 충돌체가 만들어져있는데, 충돌매니저에서 충돌체크를 실행해야하는 '주체'
	ColliderCircle* collider = GetComponent<ColliderCircle>();
	if (collider == nullptr)
	{
		collider = AddComponent<ColliderCircle>();
	}
	collider->Init(this, renderer->GetSizeX());

	_collider = collider;
	
}

void Item::Update(float deltaTime)
{
	Super::Update(deltaTime);

	Vector pos = GetPos();
	pos.y += _fallSpeed *deltaTime;
	SetPos(pos);
	
}

void Item::Render(HDC hdc)
{
	Super::Render(hdc);//
}
