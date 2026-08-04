#include "pch.h"
#include "Item.h"
#include "ImageRenderer.h"
#include "ColliderCircle.h"

void Item::Init(Vector pos, ItemKind kind)
{
	SetPos(pos);
	_kind = kind;
	ImageRenderer* renderer = GetComponent<ImageRenderer>();
	if(renderer == nullptr)
		renderer = AddComponent<ImageRenderer>();
	if(ItemKind::Power == kind)
	renderer->Init(L"PowerItem");
	else if(ItemKind::Score == kind)
	renderer->Init(L"ScoreItem");

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
	
	if ( GetPos().y > GWinSizeY)
	{
		Destroy();
	}
}

void Item::Render(HDC hdc)
{
	Super::Render(hdc);
}
