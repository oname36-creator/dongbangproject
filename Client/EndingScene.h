#pragma once

#include "Scene.h"

enum class EndingPhase
{
	Score,			// 클리어! 점수 : N 화면
	Illustration	// end01 -> end02 -> end06 삽화 슬라이드쇼
};

// 엔딩 화면. 점수 화면 -> 삽화 슬라이드쇼(2초 후 페이드아웃 x2, 마지막은 Z 입력 대기 후 페이드아웃) -> TitleScene.
class EndingScene : public Scene
{
public:
	EndingScene(int32 score = 0) : _score(score) {}

	virtual void Init() override;
	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;
	virtual void Cleanup() override;

private:
	int32 _score = 0;
	HFONT _font = nullptr;

	EndingPhase _phase = EndingPhase::Score;
	int32 _slideIndex = 0;		// 0=end01, 1=end02, 2=end06
	float _slideTimer = 0.f;
	bool _fadingOut = false;
};
