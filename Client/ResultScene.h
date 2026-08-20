#pragma once

#include "Scene.h"

// 결과 화면. 점수 표시 + Z 입력으로 TitleScene 전환하는 껍데기.
class ResultScene : public Scene
{
public:
	ResultScene(int32 score = 0) : _score(score) {}

	virtual void Init() override;
	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;
	virtual void Cleanup() override;

private:
	int32 _score = 0;
	HFONT _font = nullptr;
};
