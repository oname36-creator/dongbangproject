#include "pch.h"
#include "Enemy.h"
#include "Bullet.h"
#include "Game.h"
#include "GameScene.h"
#include "TimeManager.h"
#include "colliderCircle.h"

void Enemy::Init(Vector pos, wstring key)
{
	SetPos(pos);
	loadTexture(key);

	// 주기적으로 총알 발사하는 Timer
	_shootTimerId = TimeManager::GetInstance().AddTimer([this]() 
		{
			shootBullet();
		}, 1.0f, true);
	
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

			// 적 비행기 스스로 삭제하고
			Destroy();
			
			// 파티클 재생
			Game::GetInstance().GetScene()->CreateEffect(GetPos());

			// 점수 증가
			Game::GetInstance().GetScene()->AddScore(100);
		}
	}
}

void Enemy::shootBullet()
{
	Game::GetInstance().GetScene()->FireStraight(GetPos(), BulletType::Enemy, Vector(0, 1));
}
