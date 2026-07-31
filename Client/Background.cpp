#include "pch.h"
#include "Background.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "ImageRenderer.h"

void Background::Init(std::wstring texturKey, float moveSpeed)
{
	//_renderer = new ImageRenderer();
	ImageRenderer* renderer = AddComponent<ImageRenderer>();
	renderer->Init(texturKey);
	renderer->SetApplyCenter(false);
	_moveSpeed = moveSpeed;
	// 미리 캐싱해둔다.
	_renderer = renderer;

	//_texture = ResourceManager::GetInstance().GetTexture(L"BG");
	//_texture->SetApplyCenter(false);	// 중심좌표로 그림그리지 말라.

	// 텍스쳐의 크기를 가져와서 1,2번의 순환할 길이를 계산한다.
	_textureHeight = renderer->GetSizeY();
	_pos2 = Vector( 0, -(float)_textureHeight);

}

void Background::ChangeTexture(std::wstring textureKey)
{
	if (_renderer == nullptr)
		return;

	// 텍스처만 갈아끼우고, 순환 스크롤 위치는 처음부터 다시 계산한다.
	_renderer->Init(textureKey);
	// Init()이 내부적으로 fetch하는 Texture 객체는 _applyCenter가 각자 true로 초기화되어 있어서
	// (Background는 좌상단 기준으로 그려야 하므로) 여기서 다시 꺼줘야 한다.
	_renderer->SetApplyCenter(false);
	_textureHeight = _renderer->GetSizeY();
	SetPos(Vector(0, 0));
	_pos2 = Vector(0, -(float)_textureHeight);
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
