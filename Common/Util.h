#pragma once

float RadianToDegree(float radian);
float DegreeToRadian(float degree);

enum class BulletType
{
	Player,
	Enemy,
};

// 정지 후 재발동하는 탄이 풀릴 때 방향을 어떻게 다시 잡을지
enum class BulletRedirectMode
{
	None,	// 스폰 시 정해둔 방향 그대로 (지연만 걸림)
	Aimed,	// 발동 시점에 플레이어(적 탄) / 가장 가까운 적(플레이어 탄) 조준
	Random,	// 발동 시점에 무작위 방향으로 재설정
};

enum class RenderLayer
{
	// 아래 순서대로 렌더링이 실행된다.
	Background, // 제일 아래
	Enemy,
	Bullet,
	Player,
	Boss,
	Item,
	Effect,     // 제일 위	
	Count
};

// Actor를 판단할수있는 식별자 Type
enum class ActorType
{
	// enum은 자동으로 숫자가 +1씩 증가한다.
	Background,
	Enemy,
	PlayerBullet,
	EnemyBullet,
	Player,
	Boss,
	Effect,
	Item,
	Count,	// 최대 개수
};