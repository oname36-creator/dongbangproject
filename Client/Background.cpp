#include "pch.h"
#include "Background.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "ImageRenderer.h"

void Background::Init()
{
	//_renderer = new ImageRenderer();
	ImageRenderer* renderer = AddComponent<ImageRenderer>();
	renderer->Init(L"BG");
	renderer->SetApplyCenter(false);

	// 미리 캐싱해둔다.
	_renderer = renderer;

	//_texture = ResourceManager::GetInstance().GetTexture(L"BG");
	//_texture->SetApplyCenter(false);	// 중심좌표로 그림그리지 말라.

	// 텍스쳐의 크기를 가져와서 1,2번의 순환할 길이를 계산한다.
	_textureHeight = renderer->GetSizeY();
	_pos2 = Vector( 0, -(float)_textureHeight);
}

void Background::Update(float deltaTime)
{
	float moveY = (_moveSpeed * deltaTime);

	// 텍스처 1번 이동
	SetPos(Vector(0, GetPos().y + moveY));

	// 텍스처 2번 이동
	_pos2.y += moveY;

	// 화면 끝까지 넘어갓다면 다시 처음부터 순환
	if (GetPos().y >= _textureHeight)
	{
		SetPos(Vector(0, GetPos().y -_textureHeight * 2.0f));
	}

	if (_pos2.y >= _textureHeight)
	{
		_pos2.y -= (_textureHeight * 2);
	}
}

void Background::Render(HDC hdc)
{
	//Super::Render(hdc);

	// 두개의 텍스처를 순환해서 그린다.
	if (_renderer)
	{
		_renderer->Render(hdc, GetPos());	// 1번
		_renderer->Render(hdc, _pos2);		// 2번
	}
}
