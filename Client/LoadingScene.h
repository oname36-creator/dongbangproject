#pragma once

#include "Scene.h"

// 로딩 화면. 3초 대기 후 TitleScene으로 전환하는 껍데기.
class LoadingScene : public Scene
{
public:
	virtual void Init() override;
	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;
	virtual void Cleanup() override;

private:
	float _elapsed = 0.f;
	HFONT _font = nullptr;
};
