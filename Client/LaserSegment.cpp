#include "pch.h"
#include "LaserSegment.h"
#include "ColliderCircle.h"

void LaserSegment::Init(int radius)
{
	ColliderCircle* collider = GetComponent<ColliderCircle>();
	if (collider == nullptr)
		collider = AddComponent<ColliderCircle>();

	collider->Init(this, radius);
	collider->SetCheckCell(true);
	_collider = collider;

	// TODO: 이 세그먼트는 Laser가 매 프레임 SetPos()로 위치를 밀어넣어주는 걸 전제로 한다.
	//       (Laser::Update에서 pivot + 각도 + 세그먼트 인덱스 기반으로 위치 계산 후 여기로 전달)
}
