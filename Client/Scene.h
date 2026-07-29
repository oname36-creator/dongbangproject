#pragma once

#include "ObjectPool.h"

//#include "Enemy.h"
//#include "Bullet.h"
// C++17


// 모든 씬의 얇은 베이스. 실제 내용은 GameScene(Client/GameScene.h)에 있다.
//
// TODO(2주차 이후, TitleScene/ResultScene을 실제로 만들 때):
//  - Update(float deltaTime)를 여기 virtual로 추가할 것. 지금은 Game이 GameScene*을
//    직접 들고 있어서 필요 없지만, SceneManager가 Scene*(베이스 포인터)로 여러 씬을
//    갈아끼우게 되는 순간 Update도 다형 호출이 필요해진다.
//  - TitleScene / ResultScene을 이 클래스를 상속해서 추가한다 (배경 + 안내 문구 +
//    키 입력 정도의 껍데기면 충분하다).
//  힌트: Actor가 이미 같은 구조다(Actor.h의 virtual Update/Render를 Player/Enemy가 override).
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


