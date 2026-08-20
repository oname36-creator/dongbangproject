#pragma once

#include "Component.h"

// 텍스처 애니메이션 효과를 적용시키는 렌더러
class SpriteAnimRenderer : public Component
{
public:
	void Init(wstring textureKey);
	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc, Vector pos) override;

	bool IsEnd() const { return _isEnd; }
	void SetLoop(bool loop) { _loop = loop; }
	void SetAlpha(BYTE alpha) { _alpha = alpha; }
	void SetFlipX(bool flipX) { _flipX = flipX; }

	uint32 GetSizeX() const;
	uint32 GetSizeY() const;

private:
	class Texture* _texture = nullptr;

	// IsEnd 시점을 함수 포인터 콜백으로 알려준다.
	std::function<void()> _callBack;

	int32 _animIndexX = 0;
	int32 _animIndexY = 0;
	bool _isEnd = false;
	bool _loop = false;	// 무한 재생

	float _sumTime = 0;
	BYTE _alpha = 255;
	bool _flipX = false;
};

