#pragma once
#include "Actor.h"

// 회전하는 칼날(레이저)을 표현하는 컨트롤러.
// 실제 충돌 판정은 내부에 들고 있는 LaserSegment(ColliderCircle) 여러 개가 담당하고,
// Laser 본인은 pivot 기준 세그먼트 위치 갱신 + 칼날 도형 렌더링만 담당한다.
// pivot/각도offset/회전속도를 다르게 줘서 여러 개 조합하면(같은 pivot에 여러 자루,
// 또는 서로 다른 pivot) 레바테인 이외의 회전 칼날 패턴에도 재사용 가능하도록 설계.
class Laser : public Actor
{
	using Super = Actor;
public:
	// pivot: 회전 중심 좌표
	// angleOffset: pivot 기준 시작 각도(도)
	// length: 칼날 길이, segmentCount: 콜라이더 개수, segmentRadius: 콜라이더 하나의 반지름
	void Init(Vector pivot, float angleOffset, float length, int segmentCount, int segmentRadius);

	// 자신을 삭제 예약하기 전에, 들고 있는 LaserSegment들도 같이 삭제 예약한다.
	// (Scene은 Laser가 세그먼트를 소유한다는 걸 모르므로, 그냥 두면 세그먼트들이 풀에 반환되지 않고 계속 남는다.)
	virtual void Destroy() override;

	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;

	virtual RenderLayer GetRenderLayer() override { return RenderLayer::Enemy; }
	virtual ActorType GetActorType() override { return ActorType::Enemy; }

	// pivot이 보스를 따라가야 하면 보스 쪽에서 매 프레임 갱신
	void SetPivot(Vector pivot) { _pivot = pivot; }
	Vector GetPivot() const { return _pivot; }

	void SetAngularSpeed(float degreePerSec) { _angularSpeed = degreePerSec; }
	float GetCurrentAngle() const { return _currentAngle; }
	float GetLength() const { return _length; }
	class Texture* _bladeTexture = nullptr;
	// 보스와 맞닿는 칼날 밑동(pivot) 자리에 그려주는 장식 스프라이트. 회전 없이 pivot에 그대로 그린다. nullptr면 안 그림.
	class Texture* _guardTexture = nullptr;
private:
	Vector calcSegmentPos(int index);
	Vector _pivot;
	float _currentAngle = 0.f;	// 도(degree) 단위
	float _angularSpeed = 0.f;	// 초당 회전 각도(도), 부호로 방향 결정
	float _length = 0.f;
	int _segmentCount = 0;
	int _segmentRadius = 0;

	vector<class LaserSegment*> _segments;

	// TODO: Update()에서
	//   1) _currentAngle += _angularSpeed * deltaTime
	//   2) _pivot에서 _currentAngle 방향으로 length를 segmentCount개로 나눠서
	//      각 세그먼트 위치 계산 (Vector::Rotate 활용) 후 segment->SetPos()
	// TODO: Render()에서 _pivot ~ 칼날 끝점을 도형(Polygon/두꺼운 LineTo)으로 그리기
};
