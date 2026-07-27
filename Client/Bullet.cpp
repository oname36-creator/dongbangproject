#include "pch.h"
#include "Bullet.h"
#include "Texture.h"
#include "ResourceManager.h"
#include "ColliderCircle.h"
#include "ImageRenderer.h"

void Bullet::Init(BulletType type)
{
	_type = type;

	wstring textureKey;
	int32 textureIndex = -1;

	if (_type == BulletType::Enemy)
	{
		textureKey = L"EnemyBullet";
		// 적의 총알일 경우, sprite 5개 쪼개져있는것중에 한개 설정
		textureIndex = rand() % 5;
		_dir = Vector(0, 1); 
	}
	else
	{
		// 플레이어의 총알
		textureKey = L"PlayerBullet";
		_dir = Vector(0, -1);
	}

	//_texture = ResourceManager::GetInstance().GetTexture(textureKey);
	//_renderer = new ImageRenderer();

	// Init함수가, Pool에서 꺼내쓸때마다 호출
	// renderer 객체가 계속 생성된다.

	// 오브젝트 풀 사용시 : 예외처리 추가.
	ImageRenderer* renderer = GetComponent<ImageRenderer>();
	if (renderer == nullptr)
	{
		renderer = AddComponent<ImageRenderer>();
	}

	renderer->Init(textureKey, textureIndex);

	// texture의 width 만큼 충돌체를 생성한다.
	//_collider = new ColliderCircle();
	ColliderCircle* collider = GetComponent<ColliderCircle>();
	if (collider == nullptr)
	{
		collider = AddComponent<ColliderCircle>();
	}
	collider->Init(this, renderer->GetSizeX());

	// 플레이어의 총알만, 충돌매니저에 등록한다.
	collider->SetCheckCell(_type == BulletType::Player);

	_collider = collider;
}

void Bullet::Update(float deltaTime)
{
	// Component Update 호출을 위해
	Super::Update(deltaTime);

	Vector pos = GetPos();
	pos = pos + _dir * _moveSpeed * deltaTime;
	SetPos(pos);

	// TODO(1주차 Day5): 화면 밖 판정에 x축을 추가할 것 ★ 2주차 탄막의 전제조건
	//  현재는 y축만 검사한다. 직선탄뿐인 지금은 문제가 없지만,
	//  2주차에 부채꼴/원형 탄을 넣는 순간 좌우로 나간 탄이 '영원히 죽지 않고'
	//  오브젝트 풀을 잠식한다 → 풀 고갈 → Acquire()가 nullptr 반환 → 크래시.
	//  할 일: x < 0 || x > GWinSizeX 조건을 함께 검사한다.
	//  힌트: 경계에 여유를 조금 두면(예: -32) 화면 끝에서 탄이 깜빡 사라지는 걸 막을 수 있다.
	if (GetPos().y < 0 || GetPos().y > GWinSizeY)
	{
		// 화면 밖으로 나가면 삭제 예약
		Destroy();
	}
}

void Bullet::Render(HDC hdc)
{
	Super::Render(hdc);

	//if (_renderer)
	//{
	//	_renderer->Render(hdc, GetPos());
	//}
}
