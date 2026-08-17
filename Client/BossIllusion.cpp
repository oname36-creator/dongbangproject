#include "pch.h"
#include "BossIllusion.h"
#include "Bullet.h"
#include "Game.h"
#include "GameScene.h"
#include "Player.h"
#include "TimeManager.h"
#include "colliderCircle.h"
#include "SpriteRenderer.h"
#include <random>

static random_device rd;
static mt19937 gen(rd());

void BossIllusion::Init(Vector pos, wstring key, int32 hp, float shootOffset, wstring fanTextureKey)
{
	// 화면 위쪽(목표 x, 화면 밖 y)에서 목표 위치까지 일직선으로 내려오는 입장 연출.
	_entryTarget = pos;
	_entryStart = Vector(pos.x, -50.f);
	_entryT = 0.f;
	_isEntering = true;
	SetPos(_entryStart);

	_wanderCenter = pos;
	_wanderTargetPos = pos;
	_isDead = false;
	_hp = hp;
	_shootOffset = shootOffset;
	_baseKey = key;
	_isMoving = false;
	_fanTextureKey = fanTextureKey;

	SpriteAnimRenderer* renderer = GetComponent<SpriteAnimRenderer>();
	if (renderer == nullptr)
	{
		renderer = AddComponent<SpriteAnimRenderer>();
	}
	renderer->Init(key + L"Idle");
	renderer->SetLoop(true);
	_animRenderer = renderer;

	ColliderCircle* collider = GetComponent<ColliderCircle>();
	if (collider == nullptr)
	{
		collider = AddComponent<ColliderCircle>();
	}
	collider->Init(this, renderer->GetSizeX() * 0.7f);
	_collider = collider;

	// 배회/발사 타이머는 착지(입장 연출 완료) 시점에 beginActiveBehavior()에서 건다.
}

void BossIllusion::Destroy()
{
	Super::Destroy();

	_isDead = true;
	TimeManager::GetInstance().Remove(_wanderTimerId);
	TimeManager::GetInstance().Remove(_shootTimerId);
}

void BossIllusion::Update(float deltaTime)
{
	Super::Update(deltaTime);

	if (_isEntering)
	{
		setAnimState(true);

		_entryT += deltaTime / ENTRY_DURATION;
		if (_entryT >= 1.0f)
		{
			SetPos(_entryTarget);
			_isEntering = false;
			beginActiveBehavior();
		}
		else
		{
			SetPos(_entryStart + (_entryTarget - _entryStart) * _entryT);
		}
		return;
	}

	Vector toTarget = _wanderTargetPos - GetPos();
	float distToTarget = toTarget.Length();
	setAnimState(distToTarget > 1.0f);
	if (distToTarget > 1.0f)
	{
		Vector dir = toTarget;
		dir.Normalize();
		Vector pos = GetPos();
		pos += dir * (_moveSpeed * deltaTime);
		SetPos(pos);
	}
}

void BossIllusion::Render(HDC hdc)
{
	Super::Render(hdc);
}

void BossIllusion::OnEnter(Actor* other) // other : Bullet
{
	if (other->GetRenderLayer() == RenderLayer::Bullet)
	{
		Bullet* bullet = static_cast<Bullet*>(other);
		if (bullet && bullet->GetBulletType() == BulletType::Player)
		{
			_hp -= 1;
			bullet->Destroy();

			if (_hp <= 0)
			{
				Destroy();
				Game::GetInstance().GetScene()->CreateEffect(GetPos());
			}
		}
	}
}

void BossIllusion::pickWanderTarget()
{
	// 극좌표(각도+거리)로 뽑아야 반경 안에서 고르게 분포한다.
	// x/y를 각각 -radius~radius로 뽑으면 원이 아니라 사각형 범위가 되어 모서리 쪽으로 쏠린다.
	uniform_real_distribution<float> angleDist(0.f, 360.f);
	uniform_real_distribution<float> radiusDist(0.f, _wanderRadius);

	float radian = DegreeToRadian(angleDist(gen));
	float radius = radiusDist(gen);

	_wanderTargetPos = _wanderCenter + Vector(cosf(radian), sinf(radian)) * radius;
}

// Boss::setAnimState()와 동일한 규칙: Move는 한 번만 재생하고 마지막 프레임에서 멈춘 채로
// 이동을 계속하고, Idle은 루프.
void BossIllusion::setAnimState(bool isMoving)
{
	if (_isMoving == isMoving || _animRenderer == nullptr)
		return;

	_isMoving = isMoving;

	if (isMoving)
	{
		_animRenderer->Init(_baseKey + L"Move");
		_animRenderer->SetLoop(false);
	}
	else
	{
		_animRenderer->Init(_baseKey + L"Idle");
		_animRenderer->SetLoop(true);
	}
}

void BossIllusion::beginActiveBehavior()
{
	// _wanderRadius 안에서 배회할 목표점을 1.5초마다 재선정.
	_wanderTimerId = TimeManager::GetInstance().AddTimer([this]() { pickWanderTarget(); }, 1.5f, true);

	// _shootOffset만큼 지연 후 첫 발사, 이후 _shootInterval 주기로 반복.
	// TimeManager::AddTimer는 지연 시작을 지원하지 않아서, 1회성(loop=false) 타이머가 만료되면서
	// 그 안에서 진짜 반복 타이머를 새로 거는 식으로 우회한다.
	_shootTimerId = TimeManager::GetInstance().AddTimer([this]()
	{
		_shootTimerId = TimeManager::GetInstance().AddTimer([this]() { shootBullet(); }, _shootInterval, true);
	}, _shootOffset, false);
}

void BossIllusion::shootBullet()
{
	if (_isDead)
		return;

	GameScene* scene = Game::GetInstance().GetScene();
	if (scene == nullptr)
		return;

	// 본체(IllusionBurst)와 동일하게, Circle/Fan을 동시에 쏘지 않고 매번 무작위로 하나만 고른다.
	if (rand() % 2 == 0)
	{
		// IllusionCircle(16x16)도 Fan과 같은 이유로 자동 콜라이더 대신 절반 비율(7)을 명시한다.
		scene->FireCircle(GetPos(), BulletType::Enemy, 8, 200.f, 0.f, 0.f, BulletRedirectMode::None, L"IllusionCircle", 7.f);
	}
	else
	{
		Vector dir(0, 1);
		Player* player = scene->GetPlayer();
		if (player != nullptr)
		{
			dir = player->GetPos() - GetPos();
			dir.Normalize();
		}
		// IllusionFan 텍스처(32x32) 대비 콜라이더를 절반 정도로 명시 (자동값은 GetSizeX()-3=29로 너무 큼).
		scene->FireFan(GetPos(), BulletType::Enemy, dir, 45.f, 5, 250.f, _fanTextureKey, 14.f);
	}
}
