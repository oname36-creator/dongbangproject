#include "pch.h"
#include "Enemy.h"
#include "Bullet.h"
#include "Game.h"
#include "GameScene.h"
#include "Player.h"
#include "Item.h"
#include "TimeManager.h"
#include "colliderCircle.h"
#include "ResourceManager.h"
#include "SpriteRenderer.h"
#include "AudioManager.h"
#include <random>

static random_device rd;
static mt19937 gen(rd());

void Enemy::Init(Vector pos, wstring key, EntryDirection entryDir, float hpMultiplier)
{

	_isDead = false;

	// 항상 화면 상단 바깥의 모서리에서 출발해서, 목표까지 큰 곡선으로 가로지르며 들어온다.
	Vector spawnPos;
	switch (entryDir)
	{
		case EntryDirection::Top : spawnPos = Vector(pos.x, -50); break;
		case EntryDirection::Left : spawnPos = Vector(-50, -50); break;
		case EntryDirection::Right : spawnPos = Vector(GWinSizeX + 50, -50); break;
	}
	SetPos(spawnPos);

	// 곡선 입장: 출발점의 x와 목표의 y를 섞은 제어점을 써서 위→옆으로 휘어지게 한다.
	_isEntering = true;
	_entryT = 0.f;
	_entryStart = spawnPos;
	_entryTarget = pos;
	_entryControl = Vector(spawnPos.x, pos.y);
	// Player와 달리 Enemy는 idle 애니메이션 재생이 필요해서, Airplane::loadTexture(ImageRenderer)
	// 대신 SpriteAnimRenderer를 직접 붙인다.
	SpriteAnimRenderer* renderer = GetComponent<SpriteAnimRenderer>();
	if (renderer == nullptr)
	{
		renderer = AddComponent<SpriteAnimRenderer>();
	}
	renderer->Init(key);
	renderer->SetLoop(true);
	_animRenderer = renderer;

	ColliderCircle* collider = GetComponent<ColliderCircle>();
	if (collider == nullptr)
	{
		collider = AddComponent<ColliderCircle>();
	}
	collider->Init(this, renderer->GetSizeX() * 0.7f);
	_collider = collider;

	// 주기적으로 총알 발사하는 Timer
	_shootTimerId = TimeManager::GetInstance().AddTimer([this]() 
		{
			shootBullet();
		}, 4.0f, true);
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

_hp = (int32)(_hp * hpMultiplier);


	
}

void Enemy::Destroy()
{
	Super::Destroy();

	_isDead = true;
	// 삭제예정이니, 타이머도 같이 삭제해주자.
	TimeManager::GetInstance().Remove(_shootTimerId);
}

void Enemy::Update(float deltaTime)
{
	Super::Update(deltaTime);

	if (_isEntering)
	{
		_entryT += deltaTime / ENTRY_DURATION;
		if (_entryT >= 1.0f)
		{
			SetPos(_entryTarget);
			_isEntering = false;
		}
		else
		{
			// 2차 베지어 곡선: (1-t)^2 * start + 2(1-t)t * control + t^2 * target
			float u = 1.0f - _entryT;
			Vector pos;
			pos.x = u * u * _entryStart.x + 2.0f * u * _entryT * _entryControl.x + _entryT * _entryT * _entryTarget.x;
			pos.y = u * u * _entryStart.y + 2.0f * u * _entryT * _entryControl.y + _entryT * _entryT * _entryTarget.y;
			SetPos(pos);
		}
		return;
	}

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
			Game::GetInstance().GetScene()->CreateHitEffect(bullet->GetPos());
			_hp -= 1;
			bullet->Destroy();

			if(_hp <= 0)
			{
			Destroy();
			Game::GetInstance().GetScene()->CreateEffect(GetPos());

			AudioManager::GetInstance().Play(L"Explosion");

			// 점수 증가
			Game::GetInstance().GetScene()->AddScore(100);

			uniform_int_distribution<int> randitem(1,1000);
			int32 randnum = randitem (gen);

			if(randnum <= 400)
			{
				Game::GetInstance().GetScene()->SpawnItem(GetPos(),ItemKind::Power );
			}

			else if(randnum > 400 && randnum <= 800 )
			{
				Game::GetInstance().GetScene()->SpawnItem(GetPos(),ItemKind::Score );
			}
			else if(randnum > 800 && randnum <= 850 )
				Game::GetInstance().GetScene()->SpawnItem(GetPos(),ItemKind::Power , 8);
			else if(randnum == 851)
				Game::GetInstance().GetScene()->SpawnItem(GetPos(),ItemKind::Power , 128);
			}
			// 파티클 재생
		
		}
	}
}

void Enemy::shootBullet()
{
	if(_isDead)
		return;

	GameScene* scene = Game::GetInstance().GetScene();
	if (scene == nullptr)
		return;

	switch(_type)
	{
		case EnemyType::Circle :
			scene->FireCircle(GetPos(), BulletType::Enemy, 4, 300.f);
			break;
		case EnemyType::Fan :
			scene->FireFan(GetPos(), BulletType::Enemy, Vector(0,1), 60.f, 4, 300.f);
			break;
		case EnemyType::Aimed :
		{
			Player* player = scene->GetPlayer();
			if (player != nullptr)
			{
				scene->FireAimed(GetPos(), BulletType::Enemy, player->GetPos(), 300.f);
			}

			break;
		}
		case EnemyType::Zigzag :
			scene->FireStraight(GetPos(), BulletType::Enemy, Vector(0, 1));
			break;
	}
}
