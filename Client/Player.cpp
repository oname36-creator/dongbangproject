#include "pch.h"
#include "Player.h"
#include "InputManager.h"
#include "Game.h"
#include "GameScene.h"
#include "Enemy.h"
#include "Bullet.h"
#include "Item.h"
#include "ColliderCircle.h"
#include "ResourceManager.h"
#include "ImageRenderer.h"
#include "AudioManager.h"
#include <random>

static random_device rd;
static mt19937 gen(rd());

constexpr int32 PLAYER_IDLE_FRAME_COUNT = 4;
constexpr float PLAYER_IDLE_FRAME_DURATION = 0.15f;	// 프레임당 재생 시간(4프레임 반복이면 한 바퀴에 0.6초)

void Player::Init()
{
	loadTexture(L"Player");
	_satelliteRenderer = new ImageRenderer();
	_satelliteRenderer->Init(L"PlayerSatellite");
	// 충돌체가 만들어져있는데, 충돌매니저에서 충돌체크를 실행해야하는 '주체'
	if (_collider)
	{
		_collider->SetCheckCell(true);
		_collider->Init(this,4);
		// 스프라이트/이동은 그대로 두고, 판정 원 중심만 4px 아래로.
		_collider->SetOffset(Vector(0.f, 4.f));
	}
}

void Player::Update(float deltaTime)
{
	Super::Update(deltaTime);
	_satelliteSpacing += (subSpacing - _satelliteSpacing) * 8.f * deltaTime;   // 8.f = 수렴 속도, 취향껏 조절



	if(_invincibleTime > 0.f)
	{
		_invincibleTime -= deltaTime;
	}

	if(_fireCooldown > 0.f)
	{
		_fireCooldown -= deltaTime;
	}

	if (_subFireCooldown > 0.f)
	{
		_subFireCooldown -= deltaTime;
	}

	// 좌우 입력 없을 때(idle)만 실제로 쓰이지만, 타이머 자체는 매 프레임 그냥 흘러가게 둔다
	// (idle로 전환되는 순간 애니메이션이 항상 프레임 0부터 시작하지 않고 자연스럽게 이어지도록).
	_idleAnimTimer += deltaTime;
	if (_idleAnimTimer >= PLAYER_IDLE_FRAME_DURATION)
	{
		_idleAnimTimer -= PLAYER_IDLE_FRAME_DURATION;
		_idleAnimFrame = (_idleAnimFrame + 1) % PLAYER_IDLE_FRAME_COUNT;
	}

	_speed = _moveSpeed;
	if(InputManager::GetInstance().GetButtonPressed(KeyType::LOW_SPEED))
	{
		_speed *= 0.5f;
		subSpacing = 10.f;

	}
	else subSpacing = 30.f;
	if(InputManager::GetInstance().GetButtonDown(KeyType::BOOM) && _boom > 0)
	{
		_boom -= 1;
		Game::GetInstance().GetScene()->BombClearBullets();
		Game::GetInstance().GetScene()->ShowBombFace();
		SetInvincible(3.3f);
	}
	if (InputManager::GetInstance().GetButtonPressed(KeyType::Up))
	{
		move(0, -_speed * deltaTime);
	}

	if (InputManager::GetInstance().GetButtonPressed(KeyType::Down))
	{
		move(0, _speed * deltaTime);
	}

	if (InputManager::GetInstance().GetButtonPressed(KeyType::Left))
	{
		move(-_speed * deltaTime, 0);
	}
	else if (InputManager::GetInstance().GetButtonPressed(KeyType::Right))
	{
		move(_speed * deltaTime, 0);
	}
	else
	{
		move(0,0); 
	}
	/* 디버그 무적 토글. 필요하면 다시 주석 풀어서 쓸 것.
	if (InputManager::GetInstance().GetButtonDown(KeyType::F2))
	{
		_debugInvincible = !_debugInvincible;

	}
	*/

	if (InputManager::GetInstance().GetButtonPressed(KeyType::ATTACK) && _fireCooldown <= 0.f)
	{
		_powerLevel = getPowerStage();
		attack();
	}
	// 적비행기 가지고와서 충돌체크 수행?
}

void Player::Render(HDC hdc)
{
	if (_invincibleTime > 0.f)
	{
		int32 blinkPhase = (int32)(_invincibleTime * 20.f) % 2;
		if (blinkPhase == 0)
			return;   // 이번 프레임은 그리지 않음 -> 깜빡임
	}

	if ( _powerLevel >= 1 && _satelliteRenderer )
{
    float t = 1.f - (_satelliteSpacing - 10.f) / (30.f - 10.f);   // 0=펼쳐짐, 1=최소간격
    float headLift = t * 25.f;
	float swingBump = t * ( 1.f - t) * 4.f * 20.f;
	float arc = headLift + swingBump;                    

    Vector pos1 = GetPos();
    pos1.x -= _satelliteSpacing;
    pos1.y -= arc;
    Vector pos2 = GetPos();
    pos2.x += _satelliteSpacing;
    pos2.y -= arc;
    _satelliteRenderer->Render(hdc, pos1);
    _satelliteRenderer->Render(hdc, pos2);
}

	Super::Render(hdc);
}


void Player::OnEnter(Actor* other)
{
	// 적 총알 or 적 비행기 or 보스 몸체라면 피해입기
	if (other->GetActorType() == ActorType::Enemy ||
		other->GetActorType() == ActorType::EnemyBullet ||
		other->GetActorType() == ActorType::Boss ||
		other->GetActorType() == ActorType::EnemyLaser)
	{
		takeDamage();
	}
	else if (other->GetActorType() == ActorType::Item)
	{
		Item* item = static_cast<Item*>(other);
		if(item->GetKind() == ItemKind::Power)
		{
			if(_powerStack < 128 )
				_powerStack += item->GetPowerValue();
			if(_powerStack > 128 )
				_powerStack = 128;
		}
		else if ( item -> GetKind() == ItemKind::Score)
		{
			Game::GetInstance().GetScene()->AddScore(500);
		}
		other->Destroy();
	}
}

void Player::move(float x, float y)
{
	Vector newPos = GetPos();
	newPos.x += x;
	newPos.y += y;
	if (x < 0)
    _renderer->Init(L"Player", 0);      // 왼쪽으로 이동 중
else if (x > 0)
    _renderer->Init(L"Player", 2);      // 오른쪽으로 이동 중
else
    _renderer->Init(L"PlayerIdle", _idleAnimFrame, 0);      // 좌우 입력 없음: idle 반복 애니메이션

	// 양옆
	if (newPos.x <= GetWidth() * 0.5f)
	{
		newPos.x = (float)GetWidth() * 0.5f;
	}
	else if (newPos.x >= GWinSizeX - GetWidth() * 0.5f)
	{
		newPos.x = (float)GWinSizeX - GetWidth() * 0.5f;
	}

	// 위아래
	if (newPos.y <= GetHeight() * 0.5f)	// 이런 로직들은 world 좌표계로 생각해서 그대로 두고.
	{
		newPos.y = (float)GetHeight() * 0.5f;
	}
	else if (newPos.y >= GWinSizeY - GetHeight() * 0.5f)
	{
		newPos.y = (float)GWinSizeY - GetHeight() * 0.5f;
	}

	SetPos(newPos);
}

void Player::takeDamage()
{	
	Vector hitPos = GetPos();
	int32 _powerdrop =0;
	if(_invincibleTime > 0.f || _debugInvincible == true)
	return;

	_lives -= 1;
	if(_powerStack > 0)
	{
		if(_powerStack < 15)
		_powerdrop =_powerStack;
		else
		_powerdrop = 15;

		_powerStack -=15 ;
		if(_powerStack < 0)
			_powerStack = 0;
	}
	_invincibleTime = 3.3f;

	
	if(_powerdrop >=8)
{
	Game::GetInstance().GetScene()->SpawnItem(hitPos, ItemKind::Power, 8, true);
	for(int32 i = 0; i < (_powerdrop-8); ++i)
	{
		Game::GetInstance().GetScene()->SpawnItem(hitPos, ItemKind::Power, 1, true);
	}
}
else
{
	for(int32 i = 0; i < _powerdrop; ++i)
	{
		Game::GetInstance().GetScene()->SpawnItem(hitPos, ItemKind::Power, 1, true);
	}
}

	SetPos(Vector(GWinSizeX * 0.5f, 650.f));



	AudioManager::GetInstance().Play(L"Hit");

	// 터지는 이펙트 추가
	Game::GetInstance().GetScene()->CreateEffect(GetPos());

	// 체력이 0이면, 스스로 삭제
	if (_lives <= 0)
	{
		Destroy();
	}
}

void Player::attack()
{
		int32 shotCount = 1 + _powerLevel / 2;
		float spacing = 10.f;
		float startX = GetPos().x - spacing * (shotCount - 1)/ 2.f;

		constexpr float outerAngleDeg = 2.f;   // 양 끝 2발이 벌어지는 각도

		if (shotCount >= 3)
		{
			// 가운데(1 ~ shotCount-2번째)는 기존과 동일한 위치에서 그대로 평행 직선으로 나간다.
			for (int32 i = 1; i < shotCount - 1; ++i)
			{
				Vector firePos(startX + i * spacing, GetPos().y);
				Game::GetInstance().GetScene()->FireStraight(firePos, BulletType::Player, Vector(0, -1), 500.f);
			}

			// 양 끝(0번째, 마지막번째)은 같은 위치에서 각도만 벌어져서 나간다.
			float rad = DegreeToRadian(outerAngleDeg);
			Vector leftPos(startX, GetPos().y);
			Vector rightPos(startX + (shotCount - 1) * spacing, GetPos().y);
			Game::GetInstance().GetScene()->FireStraight(leftPos, BulletType::Player, Vector(-sinf(rad), -cosf(rad)), 500.f);
			Game::GetInstance().GetScene()->FireStraight(rightPos, BulletType::Player, Vector( sinf(rad), -cosf(rad)), 500.f);
		}
		else
		{
			for(int32 i = 0; i < shotCount; ++i)
			{
				Vector firePos(startX + i * spacing, GetPos().y);
				Game::GetInstance().GetScene()->FireStraight(firePos, BulletType::Player, Vector(0,-1), 500.f);
			}
		}
		_fireCooldown = _fireInterval;

		AudioManager::GetInstance().Play(L"Fire");

		if(_subFireCooldown <= 0.f && _powerLevel >= 1)
		{
		Vector dir(0, -1);
		Actor* target = Game::GetInstance().GetScene()->FindNearestEnemy(GetPos());
		if (target != nullptr)
		{
			dir = target->GetPos() - GetPos();
			dir.Normalize();
		}
		Vector pos1 = GetPos();
		Vector pos2 = GetPos();

		pos1.x -= _satelliteSpacing;
    	pos2.x += _satelliteSpacing;

		Game::GetInstance().GetScene()->FireHoming(pos1, BulletType::Player, dir, 400.f, 240.f);
		Game::GetInstance().GetScene()->FireHoming(pos2, BulletType::Player, dir, 400.f, 240.f);

		_subFireCooldown = _subFireInterval - (_powerLevel - 1) * 0.05f;
		if(_subFireCooldown < 0.1f)
			_subFireCooldown = 0.1f;

		
		}
}

int32 Player::getPowerStage() const
{
	
	if(_powerStack < 8)
		return 0;
	else if(_powerStack >= 8 && _powerStack < 16)
		return 1;
	else if(_powerStack >= 16 && _powerStack < 32)
		return 2;
	else if(_powerStack >= 32 && _powerStack < 48)
		return 3;
	else if(_powerStack >= 48 && _powerStack < 64)
		return 4;
	else if(_powerStack >= 64 && _powerStack < 80)
		return 5;
	else if(_powerStack >= 80 && _powerStack < 96)
		return 6;
	else if(_powerStack >= 96 && _powerStack < 128)
		return 7;
	else
		return 8;
}

Player :: ~Player()
{
	delete _satelliteRenderer;
}