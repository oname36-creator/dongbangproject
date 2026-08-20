#include "pch.h"
#include "Laser.h"
#include "LaserSegment.h"
#include "Game.h"
#include "GameScene.h"
#include "Texture.h"

void Laser::Destroy()
{
	for (LaserSegment* segment : _segments)
	{
		segment->Destroy();
	}
	_segments.clear();

	Super::Destroy();
}

void Laser::Init(Vector pivot, float angleOffset, float length, int segmentCount, int segmentRadius)
{
	_pivot = pivot;
	_currentAngle = angleOffset;
	_length = length;
	_segmentCount = segmentCount;
	_segmentRadius = segmentRadius;

	// TODO: _segmentCount 만큼 Game::GetInstance().GetScene()->CreateLaserSegment(pos, _segmentRadius) 호출해서
	//       반환된 LaserSegment*를 _segments에 채워넣기 (초기 pos는 pivot 기준으로 계산)
	//       세그먼트 간격은 length / segmentCount 가 segmentRadius*2 이하가 되게 segmentCount를 잡을 것 (틈새로 스쳐 지나가는 것 방지)
	Vector baseDir(0,1);
	Vector dir = baseDir.Rotate(DegreeToRadian(_currentAngle));

	float spacing = _length / (_segmentCount - 1);

	for(int i = 0; i < _segmentCount; ++i)
	{
		Vector pos = _pivot + dir * (spacing * i);
		LaserSegment* segment = Game::GetInstance().GetScene()->CreateLaserSegment(pos, _segmentRadius);
		if(segment == nullptr)
			break;

		_segments.push_back(segment);
	
	}
}

Vector Laser::calcSegmentPos(int index)
{
	Vector baseDir(0,1);
	Vector dir = baseDir.Rotate(DegreeToRadian(_currentAngle));
	float spacing = _length / (_segmentCount - 1 );

	return _pivot + dir * (spacing * index);
}

void Laser::Update(float deltaTime)
{
	Super::Update(deltaTime);

	_currentAngle += _angularSpeed * deltaTime;
	_currentAngle = fmodf(_currentAngle, 360.f);
	if ( _currentAngle < 0.f)
		_currentAngle += 360.f;

	for ( int i = 0; i < (int)_segments.size(); ++i)
	{
		_segments[i]->SetPos(calcSegmentPos(i));
	}

	// TODO: _currentAngle += _angularSpeed * deltaTime
	// TODO: _pivot + 현재각 방향으로 세그먼트들 위치 재계산 후 각 LaserSegment::SetPos() 호출
}

void Laser::Render(HDC hdc)
{
	if (_bladeTexture == nullptr)
		return;

		Vector tip = calcSegmentPos(_segmentCount - 1);
		Vector center = (_pivot + tip) * 0.5f;
		Vector screenCenter = Game::GetInstance().GetScene()-> ConvertWorldToScreen(center);

		float radian = DegreeToRadian(_currentAngle + 90.f);

		float thickness = (_bladeThickness > 0.f) ? _bladeThickness : (_segmentRadius * 2.f);
		_bladeTexture->RenderRotated(hdc, screenCenter, radian, Vector(_length, thickness));
	// TODO: _pivot에서 _currentAngle 방향으로 _length만큼 뻗은 칼날 도형 그리기
	//       (GDI Polygon 등으로 직접 그리기 - SetWorldTransform 회전 API는 프로젝트에 없음)

	// 칼날 밑동(pivot) 장식. 블레이드 다음에 그려서 블레이드보다 위에 보이게 한다. 원형이라 회전은 필요 없다.
	if (_guardTexture != nullptr)
	{
		Vector screenPivot = Game::GetInstance().GetScene()->ConvertWorldToScreen(_pivot);
		_guardTexture->RenderScreen(hdc, screenPivot, Vector(0, 0), Vector(40.f, 40.f));
	}
}
