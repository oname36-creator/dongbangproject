#pragma once

class Texture
{
public:
	void Load(wstring texturePath, int32 transparent, int32 row, int32 col, float dur);
	void Render(HDC hdc, Vector pos, Vector srcPos = Vector(0,0), BYTE alpha = 255);
	// destSize를 (0,0)으로 두면 원본 프레임 크기 그대로 그린다. 지정하면 그 크기로 늘려서 그린다.
	void RenderScreen(HDC hdc, Vector screenPos, Vector srcPos = Vector(0,0), Vector destSize = Vector(0,0));

	~Texture();

	uint32 GetSizeX() const { return _bitmapSizeX; }
	uint32 GetSizeY() const { return _bitmapSizeY; }
	SIZE GetFrameSize() { return SIZE(_frameSizeX, _frameSizeY); }
	void GetFrameCount(int32& outX, int32& outY) { outX = _col; outY = _row; }
	float GetDur() { return _dur; }

	void SetApplyCenter(bool apply) { _applyCenter = apply; }

private:
	HDC			_bitmapHdc = 0;
	HBITMAP		_bitmap = 0;
	int32		_transparent = -1;
	uint32		_bitmapSizeX = 0;
	uint32		_bitmapSizeY = 0;

	bool		_applyCenter = true;	// 그림을 그릴때 가운데 좌표기준으로 보정해달라.


	// sprite로 쪼개진 텍스처의 경우, 행/열 개수를 저장한다.
	int32		_col = 0;
	int32		_row = 0;
	int32		_frameSizeX = 0;
	int32		_frameSizeY = 0;
	float		_dur = 0;

	HDC 		_scratchHdc = 0;
	HBITMAP 	_scratchBitmap = 0;
	int32 		_scratchSizeX = 0;
	int32 		_scratchSizeY = 0;
};


