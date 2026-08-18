#include "pch.h"
#include "ColliderCircle.h"
#include "Actor.h"
#include "Game.h"
#include "GameScene.h"

void ColliderCircle::Init(Actor* owner, int radius)
{
    _owner = owner;
    _radius = radius;
}

Vector ColliderCircle::GetWorldPos() const
{
    if (nullptr == _owner)
        return _offset;

    return _owner->GetPos() + _offset;
}

void ColliderCircle::Render(HDC hdc, Vector pos)
{
    if (nullptr == _owner)
        return;

    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0)); // 빨간색 펜 생성
    HBRUSH hBrush = (HBRUSH)GetStockObject(NULL_BRUSH); // 투명 브러시 사용

    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

    // 원의 중심과 반지름 설정
    Vector screenPos = Game::GetInstance().GetScene()->ConvertWorldToScreen(GetWorldPos());

    int32 left = (int32)(screenPos.x - _radius);
    int32 top = (int32)(screenPos.y - _radius);
    int32 right = (int32)(screenPos.x + _radius);
    int32 bottom = (int32)(screenPos.y + _radius);

    Ellipse(hdc, left, top, right, bottom); // 원 그리기

    // 이전 GDI 객체 복원
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);

    // 생성한 GDI 객체 삭제
    DeleteObject(hPen);
}

// 충돌된 경우 true, 아니면 false
bool ColliderCircle::CheckCollision(ColliderCircle* other)
{
    // 원 vs 원 충돌 구현
    if (nullptr == other)
        return false; 

    Vector size = GetWorldPos() - other->GetWorldPos();
    float distance = size.Length();  // 두 액터의 거리 계산

    // 반지름 두개 합친 길이보다, 거리가 짧다면, 겹친것이다.
    if (distance < _radius + other->_radius)
    {
        return true;
    }
    
    return false;
}
