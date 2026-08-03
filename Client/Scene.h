#pragma once

#include "ObjectPool.h"

//#include "Enemy.h"
//#include "Bullet.h"
// C++17


// 모든 씬의 얇은 베이스. 실제 내용은 GameScene(Client/GameScene.h)에 있다.
class Scene
{
public:
	Scene();
	virtual ~Scene();
	

	// 파생 클래스가 재정의하지 않으면 아무 일도 안 하는 기본 구현.
	// virtual인데 정의가 없으면(선언만 있으면) 링크 에러가 난다 —
	// 베이스 서브오브젝트 생성/소멸 중에는 파생 클래스가 아니라 이 vtable이 쓰이기 때문에,
	// 실제로 호출되지 않더라도 심볼은 반드시 있어야 한다.
	virtual void Update(float deltaTime){};
	virtual void Init(){};
	virtual void Cleanup(){};
	virtual void Render(HDC hdc){};
};


