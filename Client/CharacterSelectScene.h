#pragma once

#include "Scene.h"

// 캐릭터 선택 화면. 현재는 레이무 하나만 있어서 좌우 선택 없이 Z로 바로 확정한다.
// 확정 시 초상화가 잠깐 반짝인 뒤 GameScene으로 전환된다.
class CharacterSelectScene : public Scene
{
public:
	virtual void Init() override;
	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;

private:
	bool _confirmed = false;
	float _flashElapsed = 0.f;
};
