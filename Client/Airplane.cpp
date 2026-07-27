#include "pch.h"
#include "Airplane.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "ColliderCircle.h"
#include "ImageRenderer.h"

void Airplane::loadTexture(wstring key)
{
	ImageRenderer* renderer = GetComponent<ImageRenderer>();
	if (renderer == nullptr)
	{
		renderer = AddComponent<ImageRenderer>();
	}

	//_renderer = new ImageRenderer();
	renderer->Init(key);

	// 매프레임 참조해야할 경우, Init 단계에서 한번 캐싱해두고 사용한다.
	_renderer = renderer;

	//_texture = ResourceManager::GetInstance().GetTexture(key);

	// texture의 width 만큼 충돌체를 생성한다.
	// 이미지 크기가 너무커서, 충돌사이즈를 적당하게 조절한다.
	//_collider = new ColliderCircle();
	ColliderCircle* collider = GetComponent<ColliderCircle>();
	if (collider == nullptr)
	{
		collider = AddComponent<ColliderCircle>();
	}
	collider->Init(this, renderer->GetSizeX() * 0.7f);

	_collider = collider;
}

void Airplane::Update(float deltaTime)
{
	// 부모 Update호출안하면, Component Update X
	Super::Update(deltaTime);
}

void Airplane::Render(HDC hdc)
{
	Super::Render(hdc);

	//if (_renderer)
	//{
	//	_renderer->Render(hdc, GetPos());
	//}
}

int32 Airplane::GetWidth()
{
	// 만약에, 매프레임 호출된다면? 
	// GetComponent 매프레임 호출
	// dynamic_cast 매프레임 호출
	// 해결 1 : 본인만의 type 체크후 static_cast -> 언리얼 Cast
	// 해결 2 : 미리 캐싱을 해둔다. -> 언리얼에서 사용, beginPlay()
	// 
	//ImageRenderer* renderer = GetComponent<ImageRenderer>();
	if (_renderer)
		return _renderer->GetSizeX();

	return 0;
}

int32 Airplane::GetHeight()
{
	if (_renderer)
		return _renderer->GetSizeY();

	return 0;
}
