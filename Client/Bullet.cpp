#include "pch.h"
#include "Bullet.h"
#include "Texture.h"
#include "ResourceManager.h"
#include "ColliderCircle.h"
#include "ImageRenderer.h"
#include "Game.h"
#include "GameScene.h"
#include "Player.h"
#include <random>

static random_device rd;
static mt19937 gen(rd());

void Bullet::Init(BulletType type, Vector dir, float speed, bool isHoming, float turnSpeed,  float accel,
				   float preStopTime, float launchDelay, BulletRedirectMode redirectMode)
{
	_type = type;
	_isHoming = isHoming;
	_turnSpeed = turnSpeed;
	_preStopTime = preStopTime;
	_launchDelay = launchDelay;
	_redirectMode = redirectMode;

	wstring textureKey;
	int32 textureIndex = -1;

	if (_type == BulletType::Enemy)
	{
		textureKey = L"EnemyBullet";
		_dir = dir;
		_moveSpeed = speed;
		_accel = accel;
	}
	else
	{
		// 플레이어의 총알 (보조 호밍탄은 다른 텍스처 사용)
		textureKey = isHoming ? L"PlayerHomingBullet" : L"PlayerBullet";
		_dir = dir;
		_moveSpeed = speed;
		_accel = accel;
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
	renderer->SetAlpha(_type == BulletType::Player ? 120 : 255);

	// texture의 width 만큼 충돌체를 생성한다.
	//_collider = new ColliderCircle();
	ColliderCircle* collider = GetComponent<ColliderCircle>();
	if (collider == nullptr)
	{
		collider = AddComponent<ColliderCircle>();
	}
	collider->Init(this, renderer->GetSizeX()-3);

	// 플레이어의 총알만, 충돌매니저에 등록한다.
	collider->SetCheckCell(_type == BulletType::Player);

	_collider = collider;
}

void Bullet::Update(float deltaTime)
{
	// Component Update 호출을 위해
	Super::Update(deltaTime);

	if (_preStopTime > 0.f)
	{
		// 정지 전까지는 그냥 평소처럼 날아간다 (아래 이동 로직으로 그대로 진행).
		_preStopTime -= deltaTime;
	}
	else if (_launchDelay > 0.f)
	{
		_launchDelay -= deltaTime;
		if (_launchDelay > 0.f)
		{
			// 아직 정지 중: 이동/화면밖 삭제 체크 없이 대기만 한다.
			return;
		}

		// 정지가 풀리는 순간, 재조준 방식에 따라 방향을 다시 잡는다.
		if (_redirectMode == BulletRedirectMode::Aimed)
		{
			Actor* target = (_type == BulletType::Enemy)
				? static_cast<Actor*>(Game::GetInstance().GetScene()->GetPlayer())
				: Game::GetInstance().GetScene()->FindNearestEnemy(GetPos());
			if (target != nullptr)
			{
				_dir = target->GetPos() - GetPos();
				_dir.Normalize();
			}
		}
		else if (_redirectMode == BulletRedirectMode::Random)
		{
			float radian = DegreeToRadian(uniform_real_distribution<float>(0.f, 360.f)(gen));
			_dir = Vector(cosf(radian), sinf(radian));
		}
	}

	if (_isHoming)
	{
		// 적 탄은 플레이어를 쫓고, 플레이어 탄(보조 공격)은 가장 가까운 적/보스를 쫓는다.
		Actor* target = (_type == BulletType::Enemy)
			? static_cast<Actor*>(Game::GetInstance().GetScene()->GetPlayer())
			: Game::GetInstance().GetScene()->FindNearestEnemy(GetPos());

		if (target != nullptr)
		{
			Vector toTarget = target->GetPos() - GetPos();
			toTarget.Normalize();

			float currentAngle = RadianToDegree(atan2f(_dir.y, _dir.x));
			float targetAngle = RadianToDegree(atan2f(toTarget.y, toTarget.x));

			// -180~180 범위로 정규화해서 최단 방향으로만 꺾이게 한다.
			float diff = targetAngle - currentAngle;
			while (diff > 180.f) diff -= 360.f;
			while (diff < -180.f) diff += 360.f;

			float maxTurn = _turnSpeed * deltaTime;
			if (diff > maxTurn) diff = maxTurn;
			else if (diff < -maxTurn) diff = -maxTurn;

			float newAngle = DegreeToRadian(currentAngle + diff);
			_dir = Vector(cosf(newAngle), sinf(newAngle));
		}
	}
		_moveSpeed = std::clamp(_moveSpeed + _accel * deltaTime, 0.f, 2000.f);
	Vector pos = GetPos();
	pos = pos + _dir * _moveSpeed * deltaTime;
	SetPos(pos);

	// 여유값(32px)만큼 화면 밖으로 나간 뒤에 삭제 : 경계에서 탄이 깜빡 사라지는 것 방지
	if (GetPos().y < -32 || GetPos().y > (GWinSizeY + 32) || GetPos().x < -32 || GetPos().x > (GWinSizeX + 32))
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
