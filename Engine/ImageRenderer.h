#pragma once

#include "Component.h"

// 단순하게 texture 애니메이션없이 그려주는 렌더러
// 비행기/ 적비행기 / 배경.. UI
class ImageRenderer : public Component
{
public:
	void Init(wstring textureKey, int32 ix = -1, int32 iy = -1);
	virtual void Render(HDC hdc, Vector pos) override;
	void SetApplyCenter(bool apply);

	uint32 GetSizeX() const;
	uint32 GetSizeY() const;

private:
	class Texture* _texture = nullptr;

	// 애니메이션 재생은 아닌데, sprite image 중에 하나를 그리고 싶을때
	int32 _iX = -1;
	int32 _iY = -1;

};

