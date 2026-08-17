#include "pch.h"
#include "Bullet.h"
#include "Texture.h"
#include "ResourceManager.h"
#include "ColliderCircle.h"
#include "SpriteRenderer.h"
#include "Game.h"
#include "GameScene.h"
#include "Player.h"
#include <random>

static random_device rd;
static mt19937 gen(rd());

void Bullet::Init(BulletType type, Vector dir, float speed, bool isHoming, float turnSpeed,  float accel,
				   float preStopTime, float launchDelay, BulletRedirectMode redirectMode,
				   wstring customTextureKey, float colliderSizeOverride, bool faceDirection, float targetSpeed,
				   float lifeTime)
{
	_type = type;
	_isHoming = isHoming;
	_turnSpeed = turnSpeed;
	_preStopTime = preStopTime;
	_launchDelay = launchDelay;
	_redirectMode = redirectMode;
	_targetSpeed = targetSpeed;
	_lifeTime = lifeTime;

	wstring textureKey;

	if (_type == BulletType::Enemy)
	{
		textureKey = customTextureKey.empty() ? L"EnemyBullet" : customTextureKey;
		_dir = dir;
		_moveSpeed = speed;
		_accel = accel;
	}
	else
	{
		// 플레이어의 총알 (보조 호밍탄은 다른 텍스처 사용)
		textureKey = customTextureKey.empty() ? (isHoming ? L"PlayerHomingBullet" : L"PlayerBullet") : customTextureKey;
		_dir = dir;
		_moveSpeed = speed;
		_accel = accel;
	}

	_faceDirection = faceDirection;
	_baseTextureKey = textureKey;

	// Init함수가, Pool에서 꺼내쓸때마다 호출
	// renderer 객체가 계속 생성된다.

	// 오브젝트 풀 사용시 : 예외처리 추가.
	SpriteAnimRenderer* renderer = GetComponent<SpriteAnimRenderer>();
	if (renderer == nullptr)
	{
		renderer = AddComponent<SpriteAnimRenderer>();
	}

	renderer->Init(textureKey);
	renderer->SetLoop(true);
	renderer->SetAlpha(_type == BulletType::Player ? 120 : 255);

	if (_faceDirection)
	{
		// renderer->Init() 직후에 회전 텍스처로 다시 덮어씌운다.
		updateFaceDirectionTexture();
	}

	// texture의 width 만큼 충돌체를 생성한다.
	//_collider = new ColliderCircle();
	ColliderCircle* collider = GetComponent<ColliderCircle>();
	if (collider == nullptr)
	{
		collider = AddComponent<ColliderCircle>();
	}
	collider->Init(this, colliderSizeOverride >= 0.f ? colliderSizeOverride : renderer->GetSizeX() - 3);

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

		if (_faceDirection)
		{
			updateFaceDirectionTexture();
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
		if (_targetSpeed >= 0.f && _accel != 0.f)
	{
		// 목표 속도가 있으면, 가속 방향과 상관없이 그 속도에 도달하는 순간 가속을 끄고 고정한다.
		float next = _moveSpeed + _accel * deltaTime;
		bool reached = (_accel > 0.f) ? (next >= _targetSpeed) : (next <= _targetSpeed);
		if (reached)
		{
			_moveSpeed = _targetSpeed;
			_accel = 0.f;
		}
		else
		{
			_moveSpeed = next;
		}
	}
	else
	{
		_moveSpeed = std::clamp(_moveSpeed + _accel * deltaTime, 0.f, 2000.f);
	}
	Vector pos = GetPos();
	pos = pos + _dir * _moveSpeed * deltaTime;
	SetPos(pos);

	// 여유값(32px)만큼 화면 밖으로 나간 뒤에 삭제 : 경계에서 탄이 깜빡 사라지는 것 방지
	if (GetPos().y < -32 || GetPos().y > (GWinSizeY + 32) || GetPos().x < -32 || GetPos().x > (GWinSizeX + 32))
	{
		// 화면 밖으로 나가면 삭제 예약
		Destroy();
	}

	// 정지해 있거나 화면을 벗어나지 않는 탄(예: 카고메 격자탄)이 무한히 쌓이는 것을 막기 위한 수명 삭제.
	if (_lifeTime >= 0.f)
	{
		_lifeTime -= deltaTime;
		if (_lifeTime <= 0.f)
		{
			Destroy();
		}
	}
}

void Bullet::SetVelocity(Vector dir, float speed)
{
	dir.Normalize();
	_dir = dir;
	_moveSpeed = speed;
	_accel = 0.f;
}

void Bullet::Render(HDC hdc)
{
	Super::Render(hdc);

	//if (_renderer)
	//{
	//	_renderer->Render(hdc, GetPos());
	//}
}

void Bullet::updateFaceDirectionTexture()
{
	SpriteAnimRenderer* renderer = GetComponent<SpriteAnimRenderer>();
	if (renderer == nullptr)
		return;

	// 텍스처를 16방향 중 가장 가까운 방향으로 회전시켜서 그린다.
	// 기본 스프라이트(0도)는 아래(0,1) 방향을 바라보고 있고, 좌우 대칭인 절반(0~180도, 9장)만
	// 미리 회전시켜 만들어뒀기 때문에 180도를 넘는 쪽은 좌우 반전으로 대체한다.
	static const wchar_t* angleLabels[9] = { L"000", L"023", L"045", L"068", L"090", L"113", L"135", L"158", L"180" };

	float theta = RadianToDegree(atan2f(-_dir.x, _dir.y));
	while (theta < 0.f) theta += 360.f;
	while (theta >= 360.f) theta -= 360.f;

	int32 bucket = (int32)(theta / 22.5f + 0.5f) % 16;
	int32 angleIndex = bucket;
	bool flipX = false;
	if (bucket > 8)
	{
		angleIndex = 16 - bucket;
		flipX = true;
	}

	renderer->Init(_baseTextureKey + L"_" + angleLabels[angleIndex]);
	renderer->SetLoop(true);
	renderer->SetAlpha(_type == BulletType::Player ? 120 : 255);
	renderer->SetFlipX(flipX);
}
