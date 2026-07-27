#include "pch.h"
#include "Player.h"
#include "InputManager.h"
#include "Game.h"
#include "Scene.h"
#include "Enemy.h"
#include "Bullet.h"
#include "ColliderCircle.h"

void Player::Init()
{
	loadTexture(L"Player");

	// 충돌체가 만들어져있는데, 충돌매니저에서 충돌체크를 실행해야하는 '주체'
	if (_collider)
	{
		_collider->SetCheckCell(true);
	}

	// TODO(1주차 Day3~4): 플레이어 히트박스를 작게 줄일 것 (기획서 4장)
	//  현재: Airplane::loadTexture()가 스프라이트 크기(renderer->GetSizeX())로 충돌 반지름을 잡는다.
	//        → 기체 그림 전체가 판정이라 탄막을 피하는 게 사실상 불가능하다.
	//  목표: 기체가 32x32px이면 판정은 '반지름 4~6px 원'. 탄막 게임의 손맛을 좌우하는 핵심이다.
	//  할 일: 여기서 _collider->Init(this, 5) 처럼 작은 값으로 다시 설정한다.
	//  참고: 적(Enemy)의 판정은 반대로 넉넉해야 잘 맞는 느낌이 난다. 플레이어만 줄여라.
}

void Player::Update(float deltaTime)
{
	Super::Update(deltaTime);

	// TODO(1주차 Day3~4): 저속 이동 구현 (기획서 5장)
	//  목표: Shift를 누르고 있는 동안 이동 속도를 40~50%로 낮춘다.
	//        탄막 사이를 정밀하게 빠져나가기 위한 필수 기능이다.
	//  할 일: 아래 이동 코드들이 _moveSpeed를 그대로 쓰고 있다.
	//        여기서 이번 프레임 속도를 한 번 계산해두고(예: speed = _moveSpeed * (느림? 0.45f : 1.f))
	//        아래 4개 move() 호출이 그 값을 쓰도록 바꾼다.
	//  선행: Engine/InputManager.h의 KeyType에 Shift가 없다. 거기부터 추가할 것.

	// TODO(1주차 Day3~4): 무적 시간 처리 (기획서 5장, 피격 후 1.5~2초)
	//  Player.h에 추가할 _invincibleTime을 여기서 deltaTime만큼 깎아준다.
	//  0보다 크면 무적 상태 → takeDamage()에서 피해를 무시한다.
	//  힌트: 무적 중에는 스프라이트를 깜빡이게 하면(짝수 프레임만 그리기) 플레이어가 상태를 알 수 있다.

	if (InputManager::GetInstance().GetButtonPressed(KeyType::W))
	{
		move(0, -_moveSpeed * deltaTime);
	}

	if (InputManager::GetInstance().GetButtonPressed(KeyType::S))
	{
		move(0, _moveSpeed * deltaTime);
	}

	if (InputManager::GetInstance().GetButtonPressed(KeyType::A))
	{
		move(-_moveSpeed * deltaTime, 0);
	}

	if (InputManager::GetInstance().GetButtonPressed(KeyType::D))
	{
		move(_moveSpeed * deltaTime, 0);
	}

	// TODO(1주차 Day3~4): 자동 연사로 바꿀 것 (기획서 5장)
	//  현재: GetButtonDown이라 '누르는 순간 딱 1발'만 나간다. 연사하려면 키를 계속 두드려야 한다.
	//  목표: 키를 누르고 있으면 일정 간격으로 계속 발사된다.
	//  할 일: _fireCooldown(float) 멤버를 두고 Update에서 deltaTime만큼 깎다가,
	//        0 이하이고 키가 눌려 있으면(GetButtonPressed) 발사 + 쿨다운 리셋.
	//  힌트: 간격 0.1초 정도부터 시작해서 감으로 조절해라.
	//  참고: 발사 키는 기획서상 Z다. SpaceBar를 Z로 바꾸려면 KeyType에 Z를 먼저 추가해야 한다.
	if (InputManager::GetInstance().GetButtonDown(KeyType::SpaceBar))
	{
		Game::GetInstance().GetScene()->CreateBullet(GetPos(), BulletType::Player);
	}

	// TODO(1주차 Day3~4): 폭탄 구현 (기획서 5장)
	//  목표: X키를 누르면 화면 안의 적 탄환을 전부 지우고, 폭탄 개수를 1 줄인다.
	//  할 일:
	//   - 폭탄이 0개면 아무 일도 일어나지 않게 막는다
	//   - GameScene에 "화면의 적 탄환을 전부 제거" 함수를 만들어 호출한다
	//  힌트: Scene::GetRenderList(RenderLayer::Bullet)로 탄 목록을 받아
	//        ActorType이 EnemyBullet인 것만 Destroy()하면 된다.
	//  함정: 순회하면서 리스트를 직접 건드리면 안 된다. Destroy()는 '삭제 예약'만 하고
	//        실제 제거는 Scene::Update() 끝에서 일어나므로 그 방식을 그대로 따르면 안전하다.
	//  참고: 보스 피해와 화면 플래시 연출은 2주차 보스 작업 때 붙여도 된다.

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

// TODO(1주차 Day3~4): 잔기 체계로 다시 쓸 것 (Player.h의 TODO와 한 세트)
//  현재: 피격당 HP -10. 10대를 맞아야 죽으므로 탄막을 피할 이유가 없다.
//  목표(기획서 5장):
//   1) 무적 시간 중이면 즉시 return (연속 피격 방지)
//   2) 잔기 1 감소
//   3) 무적 시간 1.5~2초 시작
//   4) 잔기가 남았으면 시작 위치로 되돌리고, 0이면 게임 오버를 GameScene에 알린다
//  함정: 지금처럼 Destroy()를 부르면 Player 객체가 사라진다. 그런데 Scene::_player와
//        UIManager가 이 포인터를 참조하고 있다. 잔기가 남아 있을 때는 죽이지 말고
//        '위치만 초기화'하는 편이 훨씬 안전하다.
//        (Scene::removeActor()가 _player를 nullptr로 밀어주긴 하지만, 부활 처리가 복잡해진다)
void Player::takeDamage()
{
	_hp -= 10;

	// 터지는 이펙트 추가
	Game::GetInstance().GetScene()->CreateEffect(GetPos());

	// 체력이 0이면, 스스로 삭제
	if (_hp <= 0)
	{
		Destroy();
	}
}

