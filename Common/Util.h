#pragma once

float RadianToDegree(float radian);
float DegreeToRadian(float degree);

enum class BulletType
{
	Player,
	Enemy,
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