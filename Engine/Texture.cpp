#include "pch.h"
#include "Texture.h"
#include "Game.h"
#include "GameScene.h"

void Texture::Load(wstring texturePath, int32 transparent, int32 row, int32 col, float dur)
{
	HDC hdc = ::GetDC(Game::GetInstance().GetHwnd());

	_bitmapHdc = ::CreateCompatibleDC(hdc);
	_bitmap = (HBITMAP)::LoadImageW(
		nullptr,
		texturePath.c_str(),
		IMAGE_BITMAP,
		0,
		0,
		LR_LOADFROMFILE | LR_CREATEDIBSECTION
	);

	// 투명하게 보여야하는 색상값
	_transparent = transparent;

	if (_bitmap == 0)
	{
		::MessageBox(Game::GetInstance().GetHwnd(), texturePath.c_str(), L"Invalid Texture Load", MB_OK);
		return;
	}

	HBITMAP prev = (HBITMAP)::SelectObject(_bitmapHdc, _bitmap);
	::DeleteObject(prev);

	BITMAP bit = {};
	::GetObject(_bitmap, sizeof(BITMAP), &bit);

	_bitmapSizeX = bit.bmWidth;
	_bitmapSizeY = bit.bmHeight;

	// 행/열로 쪼개진 sprite 
	_col = col;
	_row = row;
	_dur = dur;

	// 쪼개진 1개의 frame size
	_frameSizeX = _bitmapSizeX / _col;
	_frameSizeY = _bitmapSizeY / _row;

	_scratchHdc = CreateCompatibleDC(hdc);
	_scratchBitmap = CreateCompatibleBitmap(hdc, _frameSizeX, _frameSizeY);
	HBITMAP prevScratch = (HBITMAP)SelectObject(_scratchHdc, _scratchBitmap);
	DeleteObject(prevScratch);
}

void Texture::Render(HDC hdc, Vector worldPos, Vector srcPos, BYTE alpha, bool flipX)
{
	// 가운데 좌표기준으로 그림이 그려지게 보정해주자.
	Vector renderPos = Game::GetInstance().GetScene()->ConvertWorldToScreen(worldPos);
	
	if (_applyCenter)
	{
		renderPos.x -= (_frameSizeX * 0.5f);
		renderPos.y -= (_frameSizeX * 0.5f);
	}
	if ( alpha < 255)
	{
		BitBlt ( _scratchHdc, 0, 0, (int32)_frameSizeX, (int32)_frameSizeY, hdc, (int32)renderPos.x, (int32)renderPos.y, SRCCOPY);
		TransparentBlt(_scratchHdc, 0, 0, (int32)_frameSizeX, (int32)_frameSizeY,
						_bitmapHdc, (int32)srcPos.x, (int32)srcPos.y, (int32)_frameSizeX, (int32)_frameSizeY,
						_transparent);
		BLENDFUNCTION bf{};
		bf.BlendOp = AC_SRC_OVER;
		bf.BlendFlags = 0;
		bf.SourceConstantAlpha = alpha;
		bf.AlphaFormat = 0;   // 픽셀별 알파 없음 → 균일 alpha만 적용

		AlphaBlend(hdc, (int32)renderPos.x, (int32)renderPos.y, (int32)_frameSizeX, (int32)_frameSizeY,
				_scratchHdc, 0, 0, (int32)_frameSizeX, (int32)_frameSizeY, bf);

		

	}
	else if (_transparent == -1)
	{
		::BitBlt(hdc,
			(int32)renderPos.x,
			(int32)renderPos.y,
			_frameSizeX,
			_frameSizeY,
			_bitmapHdc,
			(int32)srcPos.x, //0,
			(int32)srcPos.y, //0,
			SRCCOPY);
	}
	else if (flipX)
	{
		// 좌우 반전: 스크래치 버퍼에 좌우 뒤집어서 그린 뒤(음수 width StretchBlt), 거기서 목적지로 TransparentBlt.
		::StretchBlt(_scratchHdc, (int32)_frameSizeX - 1, 0, -(int32)_frameSizeX, (int32)_frameSizeY,
			_bitmapHdc, (int32)srcPos.x, (int32)srcPos.y, (int32)_frameSizeX, (int32)_frameSizeY, SRCCOPY);
		::TransparentBlt(hdc,
			(int32)renderPos.x,
			(int32)renderPos.y,
			_frameSizeX,
			_frameSizeY,
			_scratchHdc,
			0,
			0,
			_frameSizeX,
			_frameSizeY,
			_transparent);
	}
	else
	{
		::TransparentBlt(hdc,
			(int32)renderPos.x,	// 윈도우 좌표 어디에 그릴지
			(int32)renderPos.y, // 윈도우 좌표 어디에 그릴지
			_frameSizeX, //_bitmapSizeX,       // 윈도우 좌표에 그려질 최종 크기
			_frameSizeY, //_bitmapSizeY,		// 윈도우 좌표에 그려질 최종 크기
			_bitmapHdc,			// 해당 비트맵을 그려줘
			(int32)srcPos.x, //0,					// 그리고 싶은 비트맵의 좌표
			(int32)srcPos.y, //0,					// 그리고 싶은 비트맵의 좌표
			_frameSizeX, //_bitmapSizeX,		// 그리고 싶은 비트맵의 크기
			_frameSizeY, //_bitmapSizeY,		// 그리고 싶은 비트맵의 크기
			_transparent);		// 투명 키값(RGB)
	}
}


void Texture::RenderScreen(HDC hdc, Vector screenPos, Vector srcPos, Vector destSize, BYTE alpha)
{
	int32 destSizeX = (destSize.x > 0) ? (int32)destSize.x : _frameSizeX;
	int32 destSizeY = (destSize.y > 0) ? (int32)destSize.y : _frameSizeY;

	if (_applyCenter)
	{
		screenPos.x -= (destSizeX * 0.5f);
		screenPos.y -= (destSizeY * 0.5f);
	}

	if (alpha < 255 && _transparent != -1)
	{
		// 목적 크기(destSizeX/Y)로 늘려 그려야 하는데다 반투명까지 있어서, 고정 크기 스크래치 버퍼로는
		// 부족할 수 있다. 이 호출에 한해 임시 버퍼를 만들어 배경 캡처 -> 확대+컬러키 그리기 -> AlphaBlend 순으로 합성한다.
		HDC tempDC = CreateCompatibleDC(hdc);
		HBITMAP tempBmp = CreateCompatibleBitmap(hdc, destSizeX, destSizeY);
		HBITMAP prevTempBmp = (HBITMAP)SelectObject(tempDC, tempBmp);

		BitBlt(tempDC, 0, 0, destSizeX, destSizeY, hdc, (int32)screenPos.x, (int32)screenPos.y, SRCCOPY);
		TransparentBlt(tempDC, 0, 0, destSizeX, destSizeY,
			_bitmapHdc, (int32)srcPos.x, (int32)srcPos.y, _frameSizeX, _frameSizeY, _transparent);

		BLENDFUNCTION bf{};
		bf.BlendOp = AC_SRC_OVER;
		bf.BlendFlags = 0;
		bf.SourceConstantAlpha = alpha;
		bf.AlphaFormat = 0;
		AlphaBlend(hdc, (int32)screenPos.x, (int32)screenPos.y, destSizeX, destSizeY, tempDC, 0, 0, destSizeX, destSizeY, bf);

		SelectObject(tempDC, prevTempBmp);
		DeleteObject(tempBmp);
		DeleteDC(tempDC);
	}
	else if (_transparent == -1)
	{
		::StretchBlt(hdc,
			(int32)screenPos.x,
			(int32)screenPos.y,
			destSizeX,
			destSizeY,
			_bitmapHdc,
			(int32)srcPos.x,
			(int32)srcPos.y,
			_frameSizeX,
			_frameSizeY,
			SRCCOPY);
	}
	else
	{
		::TransparentBlt(hdc,
			(int32)screenPos.x,
			(int32)screenPos.y,
			destSizeX,
			destSizeY,
			_bitmapHdc,
			(int32)srcPos.x,
			(int32)srcPos.y,
			_frameSizeX,
			_frameSizeY,
			_transparent);
	}
}

Texture::~Texture()
{
	DeleteDC(_scratchHdc);
	DeleteObject(_scratchBitmap);
}
