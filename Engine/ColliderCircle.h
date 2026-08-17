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

private:
	Actor*		_owner = nullptr;
	int32		_radius;	// 반지름

	// 충돌 매니저에서, 실제로 충돌을 실행하는 '주체'
	bool		_checkCell = false;
};

