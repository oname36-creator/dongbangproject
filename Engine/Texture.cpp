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

void Texture::RenderRotated(HDC hdc, Vector centerPos, float radian, Vector destSize)
{
	if (destSize == Vector())
		destSize = Vector((float)_bitmapSizeX, (float)_bitmapSizeY);

	// 회전 전 로컬 기준의 3개 꼭짓점 계산 좌상, 우상, 좌하 순
	float halfX = destSize.x / 2.f;
	float halfY = destSize.y / 2.f;

	Vector localCorners[3] = {
		{ -halfX, -halfY }, // 좌상
		{  halfX, -halfY }, // 우상
		{ -halfX,  halfY }  // 좌하
	};

	// 삼각함수를 이용하여 3개 꼭짓점을 중심점(centerPos) 기준으로 월드 회전 변환
	POINT destPoints[3];

	for (int i = 0; i < 3; ++i)
	{
		Vector rotatePos = localCorners[i].Rotate(radian);

		destPoints[i].x = static_cast<long>(rotatePos.x + centerPos.x);
		destPoints[i].y = static_cast<long>(rotatePos.y + centerPos.y);
	}

	// 투명키 배경을 지우며 회전한다.
	if (_transparent != -1)
	{
		// 회전 시 이미지가 빠져나가지 않도록 충분한 비트맵 대각선 크기 계산
		int32 maxDim = static_cast<int32>(sqrt(destSize.x * destSize.x + destSize.y * destSize.y)) + 4;

		if (!_tempDC || maxDim > _tempDim)
		{
			if (_tempDC) { ::DeleteDC(_tempDC); _tempDC = nullptr; }
			if (_tempBitmap) { ::DeleteObject(_tempBitmap); _tempBitmap = nullptr; }

			_tempDim = maxDim;
			_tempDC = ::CreateCompatibleDC(hdc);
			_tempBitmap = ::CreateCompatibleBitmap(hdc, _tempDim, _tempDim);
			::SelectObject(_tempDC, _tempBitmap);
		}

		// 임시 버퍼의 배경을 투명 키값으로 통일해서 채워둔다.
		HBRUSH bgBrush = ::CreateSolidBrush(_transparent);
		RECT r = { 0, 0, _tempDim, _tempDim };
		::FillRect(_tempDC, &r, bgBrush);
		::DeleteObject(bgBrush);

		// 임시 버퍼 내부의 정중앙에 회전된 꼭짓점이 맺히도록 오프셋 좌표 보정
		POINT rotatedDestPoints[3];
		float tempHalf = _tempDim / 2.f;

		for (int i = 0; i < 3; ++i)
		{
			Vector rotatePos = localCorners[i].Rotate(radian);
			rotatedDestPoints[i].x = static_cast<long>(rotatePos.x + tempHalf);
			rotatedDestPoints[i].y = static_cast<long>(rotatePos.y + tempHalf);
		}

		::SetStretchBltMode(_tempDC, COLORONCOLOR);
		// 임시 버퍼에 원본을 회전시켜서 찍는다.
		::PlgBlt(_tempDC, rotatedDestPoints, _bitmapHdc, 0, 0, _bitmapSizeX, _bitmapSizeY, NULL, 0, 0);

		// 배경 처리가 끝난 tempDC 자체를 실제 화면에 옮긴다 (여기서는 이미 회전된 결과라 그대로 복사만 하면 됨).
		int32 drawX = (int32)(centerPos.x - tempHalf);
		int32 drawY = (int32)(centerPos.y - tempHalf);

		::TransparentBlt(hdc, drawX, drawY, _tempDim, _tempDim,
			_tempDC, 0, 0, _tempDim, _tempDim, _transparent);
	}
	else
	{
		::SetStretchBltMode(hdc, COLORONCOLOR);
		// 투명 키값이 없는 이미지라면 마스크 없이 원본 그대로 바로 PlgBlt 회전 수행
		::PlgBlt(hdc, destPoints, _bitmapHdc, 0, 0, _bitmapSizeX, _bitmapSizeY, NULL, 0, 0);
	}
}

Texture::~Texture()
{
	DeleteDC(_scratchHdc);
	DeleteObject(_scratchBitmap);
	DeleteDC(_tempDC);
	DeleteObject(_tempBitmap);
}
