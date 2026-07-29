#pragma once

#include "Scene.h"

// 타이틀 화면. 배경 + 안내 문구 + Z 입력으로 GameScene 전환하는 껍데기.
class TitleScene : public Scene
{
public:
	virtual void Init() override;
	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;
};
