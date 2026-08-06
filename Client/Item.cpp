#include "pch.h"
#include "Item.h"
#include "ImageRenderer.h"
#include "ColliderCircle.h"
#include "Game.h"
#include "GameScene.h"
#include "Player.h"
#include <random>

static random_device rd;
static mt19937 gen(rd());

void Item::Init(Vector pos, ItemKind kind, int32 powerValue, bool burst, bool autoCollect)
{
	SetPos(pos);
	_kind = kind;
	_powerValue = powerValue;
	_autoCollect = autoCollect;

	if (burst)
	{
		// 위로 발사됐다가 중력에 의해 서서히 느려지고, 다시 천천히 떨어진다.
		uniform_int_distribution<int> vxDist(-150, 150);
		uniform_int_distribution<int> vyDist(-550, -350);
		_velocityX = (float)vxDist(gen);
		_velocityY = (float)vyDist(gen);
	}
	else
	{
		_velocityX = 0.f;
		_velocityY = 0.f;
	}

	ImageRenderer* renderer = GetComponent<ImageRenderer>();
	if(renderer == nullptr)
		renderer = AddComponent<ImageRenderer>();
	if(ItemKind::Power == kind)
	renderer->Init(powerValue >= 128 ? L"FullPowerItem"
		 : powerValue >= 8 ? L"BigPowerItem" : L"PowerItem");
	else if(ItemKind::Score == kind)
	renderer->Init(L"ScoreItem");

	// 충돌체가 만들어져있는데, 충돌매니저에서 충돌체크를 실행해야하는 '주체'
	ColliderCircle* collider = GetComponent<ColliderCircle>();
	if (collider == nullptr)
	{
		collider = AddComponent<ColliderCircle>();
	}
	collider->Init(this, renderer->GetSizeX()*2);

	_collider = collider;
	
}

void Item::Update(float deltaTime)
{
	Super::Update(deltaTime);

	if (_autoCollect)
	{
		Player* player = Game::GetInstance().GetScene()->GetPlayer();
		if (player != nullptr)
		{
			Vector dir = player->GetPos() - GetPos();
			dir.Normalize();

			Vector pos = GetPos();
			pos.x += dir.x * _collectSpeed * deltaTime;
			pos.y += dir.y * _collectSpeed * deltaTime;
			SetPos(pos);
		}
		return;
	}

	// 중력: _velocityY가 위(음수)에서 시작해도 서서히 _fallSpeed(느린 하강)까지만 가속된다.
	_velocityY += _gravity * deltaTime;
	if (_velocityY > _fallSpeed)
		_velocityY = _fallSpeed;

	Vector pos = GetPos();
	pos.x += _velocityX * deltaTime;
	pos.y += _velocityY * deltaTime;

	if(pos.x < 0)
	{
		pos.x = 0;
		_velocityX = 0.f;
	}
	else if (pos.x > (float)GWinSizeX)
	{
		pos.x = (float)GWinSizeX;
		_velocityX = 0.f;
	}

	SetPos(pos);

	if ( GetPos().y > GWinSizeY)
	{
		Destroy();
	}
}

void Item::Render(HDC hdc)
{
	Super::Render(hdc);
}
