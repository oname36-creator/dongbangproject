#include "pch.h"
#include "Background.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "ImageRenderer.h"

void Background::Init(std::wstring texturKey, float moveSpeed, float moveSpeedX)
{
	//_renderer = new ImageRenderer();
	ImageRenderer* renderer = AddComponent<ImageRenderer>();
	renderer->Init(texturKey);
	renderer->SetApplyCenter(false);
	_moveSpeed = moveSpeed;
	_moveSpeedX = moveSpeedX;
	// 미리 캐싱해둔다.
	_renderer = renderer;

	//_texture = ResourceManager::GetInstance().GetTexture(L"BG");
	//_texture->SetApplyCenter(false);	// 중심좌표로 그림그리지 말라.

	// 텍스쳐의 크기를 가져와서 1,2번의 순환할 길이를 계산한다.
	_textureHeight = renderer->GetSizeY();
	_textureWidth = renderer->GetSizeX();
	_pos2 = Vector( 0, -(float)_textureHeight);
	_scrollX = 0.f;

}

void Background::ChangeTexture(std::wstring textureKey, float moveSpeedX, float moveSpeed)
{
	if (_renderer == nullptr)
		return;

	// 텍스처만 갈아끼우고, 순환 스크롤 위치는 처음부터 다시 계산한다.
	_renderer->Init(textureKey);
	// Init()이 내부적으로 fetch하는 Texture 객체는 _applyCenter가 각자 true로 초기화되어 있어서
	// (Background는 좌상단 기준으로 그려야 하므로) 여기서 다시 꺼줘야 한다.
	_renderer->SetApplyCenter(false);
	_textureHeight = _renderer->GetSizeY();
	_textureWidth = _renderer->GetSizeX();
	_moveSpeedX = moveSpeedX;
	// moveSpeed를 -1(기본값)로 넘기면 세로 속도는 기존 값 유지, 그 외엔 새 값으로 교체
	if (moveSpeed >= 0.f)
		_moveSpeed = moveSpeed;
	SetPos(Vector(0, 0));
	_pos2 = Vector(0, -(float)_textureHeight);
	_scrollX = 0.f;
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

	// 가로 스크롤 진행 + 순환 (텍스처 폭 기준)
	_scrollX += (_moveSpeedX * deltaTime);
	if (_textureWidth > 0)
	{
		if (_scrollX >= _textureWidth)
			_scrollX -= _textureWidth;
		if (_scrollX < 0)
			_scrollX += _textureWidth;
	}
}

void Background::Render(HDC hdc)
{
	//Super::Render(hdc);

	if (_renderer == nullptr)
		return;

	if (_moveSpeedX == 0.f)
	{
		// 가로 스크롤이 없으면 기존과 동일하게 세로 2장만 그린다.
		_renderer->Render(hdc, GetPos());
		_renderer->Render(hdc, _pos2);
		return;
	}

	// 가로로 스크롤할 때는 복사본이 필드 폭(GWinSizeX)을 넘어 사이드바를 덮지 않도록
	// 필드 영역으로 클리핑한 뒤, 세로 2장 x 가로 2장(총 4장)을 순환해서 그린다.
	int32 savedDC = ::SaveDC(hdc);
	::IntersectClipRect(hdc, 0, 0, GWinSizeX, GWinSizeY);

	{
		float xs[2] = { -_scrollX, -_scrollX + (float)_textureWidth };
		float ys[2] = { GetPos().y, _pos2.y };
		for (int32 yi = 0; yi < 2; ++yi)
		{
			for (int32 xi = 0; xi < 2; ++xi)
			{
				_renderer->Render(hdc, Vector(xs[xi], ys[yi]));
			}
		}
	}

	::RestoreDC(hdc, savedDC);
}
