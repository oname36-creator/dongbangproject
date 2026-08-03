#include "pch.h"
#include "Player.h"
#include "InputManager.h"
#include "Game.h"
#include "GameScene.h"
#include "Enemy.h"
#include "Bullet.h"
#include "ColliderCircle.h"
#include "ResourceManager.h"

void Player::Init()
{
	loadTexture(L"Player");

	// 충돌체가 만들어져있는데, 충돌매니저에서 충돌체크를 실행해야하는 '주체'
	if (_collider)
	{
		_collider->SetCheckCell(true);
		_collider->Init(this,4);
	}
}

void Player::Update(float deltaTime)
{
	Super::Update(deltaTime);

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

	_speed = _moveSpeed;
	if(InputManager::GetInstance().GetButtonPressed(KeyType::LOW_SPEED))
	{
		_speed *= 0.5f;
	}
	if(InputManager::GetInstance().GetButtonDown(KeyType::BOOM))
	{
		_boom -= 1;
		Game::GetInstance().GetScene()->ClearEnemyBullets();
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

	if (InputManager::GetInstance().GetButtonPressed(KeyType::Right))
	{
		move(_speed * deltaTime, 0);
	}

	if (InputManager::GetInstance().GetButtonDown(KeyType::F2))
	{
		_debugInvincible = !_debugInvincible;
		
	}

	if (InputManager::GetInstance().GetButtonPressed(KeyType::ATTACK) && _fireCooldown <= 0.f)
	{

		Game::GetInstance().GetScene()->FireStraight(GetPos(), BulletType::Player, Vector(0,-1));
		_fireCooldown = _fireInterval;

		fs::path firePath = ResourceManager::GetInstance().GetResourcePath() / L"Fire.wav";
		::PlaySound(firePath.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
	}

	// 보조 공격: SpaceBar. 가장 가까운 적을 향해 유도탄을 쏜다.
	if (InputManager::GetInstance().GetButtonPressed(KeyType::SpaceBar) && _subFireCooldown <= 0.f)
	{
		Vector dir(0, -1);
		Actor* target = Game::GetInstance().GetScene()->FindNearestEnemy(GetPos());
		if (target != nullptr)
		{
			dir = target->GetPos() - GetPos();
			dir.Normalize();
		}

		Game::GetInstance().GetScene()->FireHoming(GetPos(), BulletType::Player, dir, 400.f, 240.f);
		_subFireCooldown = _subFireInterval;
	}

	// 적비행기 가지고와서 충돌체크 수행?
}

void Player::Render(HDC hdc)
{
	Super::Render(hdc);
}

void Player::OnEnter(Actor* other)
{
	// 적 총알 or 적 비행기라면 피해입기
	if (other->GetActorType() == ActorType::Enemy ||
		other->GetActorType() == ActorType::EnemyBullet)
	{
		takeDamage();
	}
}

void Player::move(float x, float y)
{
	Vector newPos = GetPos();
	newPos.x += x;
	newPos.y += y;

	// 양옆
	if (newPos.x <= GetWidth())
	{
		newPos.x = (float)GetWidth();
	}
	else if (newPos.x >= GWinSizeX - GetWidth())
	{
		newPos.x = (float)GWinSizeX - GetWidth();
	}

	// 위아래
	if (newPos.y <= GetHeight())	// 이런 로직들은 world 좌표계로 생각해서 그대로 두고.
	{
		newPos.y = (float)GetHeight();
	}
	else if (newPos.y >= GWinSizeY - GetHeight())
	{
		newPos.y = (float)GWinSizeY - GetHeight();
	}

	SetPos(newPos);
}

void Player::takeDamage()
{
	if(_invincibleTime > 0.f || _debugInvincible == true)
	return;

	_lives -= 1;
	_invincibleTime = 1.5f;

	fs::path hitPath = ResourceManager::GetInstance().GetResourcePath() / L"Hit.wav";
	::PlaySound(hitPath.c_str(), nullptr, SND_FILENAME | SND_ASYNC);

	// 터지는 이펙트 추가
	Game::GetInstance().GetScene()->CreateEffect(GetPos());

	// 체력이 0이면, 스스로 삭제
	if (_lives <= 0)
	{
		Destroy();
	}
}

