#pragma once

#include "Component.h"

class ColliderCircle : public Component
{
public:
	void Init(class Actor* owner, int radius);
	virtual void Render(HDC hdc, Vector pos) override;

	// 실제로 충돌체크가 필요한 셀인지
	bool CheckCell() { return _checkCell; }
	void SetCheckCell(bool flag) { _checkCell = flag; }

	bool CheckCollision(ColliderCircle* other);

	int32 GetRadius() const { return _radius; }

	// 소유자 스프라이트/이동은 그대로 두고, 판정 원의 중심만 소유자 위치에서 이만큼 옮기고 싶을 때 사용.
	void SetOffset(Vector offset) { _offset = offset; }
	Vector GetWorldPos() const;

private:
	Actor*		_owner = nullptr;
	int32		_radius;	// 반지름
	Vector		_offset;	// 소유자 위치 기준 판정 원 중심 오프셋. 기본값(0,0)이면 기존과 동일.

	// 충돌 매니저에서, 실제로 충돌을 실행하는 '주체'
	bool		_checkCell = false;
};

