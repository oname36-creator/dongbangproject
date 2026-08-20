#include "pch.h"
#include "ImageRenderer.h"
#include "Texture.h"
#include "ResourceManager.h"

void ImageRenderer::Init(wstring textureKey, int32 ix, int32 iy)
{
	_texture = ResourceManager::GetInstance().GetTexture(textureKey);
	_iX = ix;
	_iY = iy;
}

void ImageRenderer::Render(HDC hdc, Vector pos)
{
	if (_texture)
	{
		// sprite animation 아니라, 단순 이미지인데 sprite로 쪼개져있는 경우
		// 예) EnemyBullet
		Vector srcPos(0, 0);
		if (_iX != -1)
		{
			srcPos.x = (float)_iX * _texture->GetFrameSize().cx;
		}
		if (_iY != -1)
		{
			srcPos.y = (float)_iY * _texture->GetFrameSize().cy;
		}

		_texture->Render(hdc, pos, srcPos, _alpha);
	}
}

uint32 ImageRenderer::GetSizeX() const
{
	if (_texture)
	{
		return _texture->GetFrameSize().cx;
	}

	return 0;
}

uint32 ImageRenderer::GetSizeY() const
{
	if (_texture)
	{
		return _texture->GetFrameSize().cy;
	}

	return 0;
}

void ImageRenderer::SetApplyCenter(bool apply)
{
	if (_texture)
	{
		_texture->SetApplyCenter(apply);
	}
}