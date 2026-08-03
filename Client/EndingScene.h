#pragma once

#include "Scene.h"

// 엔딩 화면. 축하 문구 + 점수 표시 + Z 입력으로 TitleScene 전환하는 껍데기.
class EndingScene : public Scene
{
public:
	EndingScene(int32 score = 0) : _score(score) {}

	virtual void Init() override;
	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;

private:
	int32 _score = 0;
};
