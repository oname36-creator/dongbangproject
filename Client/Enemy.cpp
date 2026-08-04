#include "pch.h"
#include "Enemy.h"
#include "Bullet.h"
#include "Game.h"
#include "GameScene.h"
#include "Player.h"
#include "TimeManager.h"
#include "colliderCircle.h"
#include "ResourceManager.h"
#include <random>

static random_device rd;
static mt19937 gen(rd());

void Enemy::Init(Vector pos, wstring key)
{
	SetPos(pos);
	loadTexture(key);

	// 주기적으로 총알 발사하는 Timer
	_shootTimerId = TimeManager::GetInstance().AddTimer([this]() 
		{
			shootBullet();
		}, 2.0f, true);
	EnemyType type = EnemyType::Aimed;
if (key == L"Enemy1")
	{
	type = EnemyType::Zigzag;
	_hp = 2;
	}
else if (key == L"Enemy2")
	{
	type = EnemyType::Fan;
	_hp = 2;
	}

else if (key == L"Enemy3")
	{
    type = EnemyType::Circle;
	_hp = 2;
	}
else if (key == L"Enemy4")
	{
    type = EnemyType::Aimed;
	_hp = 2;
	}
else
	{
	// 알 수 없는 key: 웨이브 테이블 오타 등. 조용히 넘어가지 않도록 알린다.
	assert(false && "Enemy::Init - unknown enemy key");
	}
_type = type;


	
}

void Enemy::Destroy()
{
	Super::Destroy();


	// 삭제예정이니, 타이머도 같이 삭제해주자.
	TimeManager::GetInstance().Remove(_shootTimerId);
}

void Enemy::Update(float deltaTime)
{
	Super::Update(deltaTime);

	// 좌우로 움직이며 밑으로 내려온다.
	float x = _moveSpeedX * deltaTime * sinf(_sumRadian);
	float y = _moveSpeedY * deltaTime;
	
	Vector pos = GetPos();
	pos.x += x;
	pos.y += y;
	SetPos(pos);

	_sumRadian += (_turnSpeed * deltaTime);

	if (GetPos().y > GWinSizeY)
	{
		// 화면 밖으로 나가면 삭제하자
		Destroy();	// 언리얼 방식
	}
}

void Enemy::Render(HDC hdc)
{
	Super::Render(hdc);
}

void Enemy::OnEnter(Actor* other) // other : Player
{
	
	// 무언가와 '처음으로' 충돌되었다.
	// '누구'와 정확하게 충돌되었는지 판단하자.
	// 1번 방식
	//static_cast<Bullet*>(other) : 안전한 캐스팅은 아니다. player 타입도 무조건 Bullet으로 캐스팅해준다.
	Bullet* bullet = dynamic_cast<Bullet*>(other);
	if (bullet != nullptr)
	{

	}

	// 2번 방식 : 먼저 타입을 확인후, static_cast 수행
	if (other->GetRenderLayer() == RenderLayer::Bullet)
	{
		Bullet* bullet = static_cast<Bullet*>(other);
		if (bullet && bullet->GetBulletType() == BulletType::Player)
		{
			// 플레이어의 총알이다.
			_hp -= 1;
			bullet->Destroy();

			if(_hp <= 0)
			{
			Destroy();
			Game::GetInstance().GetScene()->CreateEffect(GetPos());

			fs::path explosionPath = ResourceManager::GetInstance().GetResourcePath() / L"Explosion.wav";
			::PlaySound(explosionPath.c_str(), nullptr, SND_FILENAME | SND_ASYNC);

			// 점수 증가
			Game::GetInstance().GetScene()->AddScore(100);

			uniform_int_distribution<int> randitem(1,10);
			int32 randnum = randitem (gen);

			if(randnum < 3)
			{
				Game::GetInstance().GetScene()->SpawnItem(GetPos());
			}
			}
			// 파티클 재생
		
		}
	}
}

void Enemy::shootBullet()
{
	switch(_type)
	{
		case EnemyType::Circle : 
			Game::GetInstance().GetScene()->FireCircle(GetPos(), BulletType::Enemy, 4, 300.f);
			break;
		case EnemyType::Fan : 
			Game::GetInstance().GetScene()->FireFan(GetPos(), BulletType::Enemy, Vector(0,1), 60.f, 4, 300.f);
			break;
		case EnemyType::Aimed :
		{
			Player* player = Game::GetInstance().GetScene()->GetPlayer();
			if (player != nullptr)
			{
				Game::GetInstance().GetScene()->FireAimed(GetPos(), BulletType::Enemy, player->GetPos(), 300.f);
			}
		
			break;
		}
		case EnemyType::Zigzag :
			Game::GetInstance().GetScene()->FireStraight(GetPos(), BulletType::Enemy, Vector(0, 1));
			break;
	}
}
