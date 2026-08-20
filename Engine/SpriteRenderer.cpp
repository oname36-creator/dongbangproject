#include "pch.h"
#include "SpriteRenderer.h"
#include "ResourceManager.h"
#include "Texture.h"

void SpriteAnimRenderer::Init(wstring textureKey)
{
	_texture = ResourceManager::GetInstance().GetTexture(textureKey);
	_animIndexX = 0;
	_animIndexY = 0;
	_sumTime = 0;
	_isEnd = false;
	_flipX = false;
}

void SpriteAnimRenderer::Update(float deltaTime)
{
	if (_isEnd)
		return;

	if (_texture == nullptr)
		return;

	if (_texture->GetDur() <= 0)
		return;

	_sumTime += deltaTime;

	int32 frameCountX = 0;
	int32 frameCountY = 0;
	_texture->GetFrameCount(frameCountX, frameCountY);

	int32 totalCount = frameCountX * frameCountY;
	float frameTime = _texture->GetDur() / totalCount;

	// 일정 시간이 지나면 다음 프레임 이동
	if (_sumTime >= frameTime)
	{
		_sumTime -= frameTime;

		// 현재 인덱스를 선형적으로 계산 (fullFrame 여부에 따라 범위가 달라짐)
		int32 currentIndex = (_animIndexY * frameCountX + _animIndexX);
		int32 nextIndex = currentIndex + 1;

		if (nextIndex >= totalCount)
		{
			if (_loop)
			{
				nextIndex = 0;
			}
			else
			{
				_isEnd = true;

				// 함수 포인터 호출
				if (_callBack)
				{
					_callBack();
				}
				return; // 마지막 프레임 유지
			}
		}

		// 인덱스 업데이트
		_animIndexX = nextIndex % frameCountX;
		_animIndexY = nextIndex / frameCountX;
	}
}

void SpriteAnimRenderer::Render(HDC hdc, Vector pos)
{
	if (nullptr == _texture)
		return;

	SIZE frameSize = _texture->GetFrameSize();

	// 소스 비트맵에서 복사할 시작 좌표 계산
	float srcX = _animIndexX * (float)frameSize.cx;
	float srcY = _animIndexY * (float)frameSize.cy;

	_texture->Render(hdc, pos, Vector(srcX, srcY), _alpha, _flipX);
}

uint32 SpriteAnimRenderer::GetSizeX() const
{
	if (_texture)
		return _texture->GetFrameSize().cx;

	return 0;
}

uint32 SpriteAnimRenderer::GetSizeY() const
{
	if (_texture)
		return _texture->GetFrameSize().cy;

	return 0;
}