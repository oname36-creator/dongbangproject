#pragma once

#include "Scene.h"

enum class EndingPhase
{
	Score,			// 클리어! 점수 : N 화면
	Illustration,	// end01 -> end02 -> end06 삽화 슬라이드쇼
	ExtraCredit		// 엑스트라 클리어 전용: 검은 화면 + 큰 흰 글씨 문구를 3초간 보여주고 바로 타이틀로
};

// 엔딩 화면. 일반 클리어: 점수 화면 -> 삽화 슬라이드쇼(2초 후 페이드아웃 x2, 마지막은 Z 입력 대기 후 페이드아웃) -> TitleScene.
// 엑스트라 클리어: 점수/삽화 없이 검은 화면 + 문구 3초 -> TitleScene.
class EndingScene : public Scene
{
public:
	EndingScene(int32 score = 0, bool isExtra = false) : _score(score), _isExtra(isExtra) {}

	virtual void Init() override;
	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;
	virtual void Cleanup() override;

private:
	int32 _score = 0;
	bool _isExtra = false;
	HFONT _font = nullptr;
	HFONT _creditFont = nullptr;
	HFONT _smallCreditFont = nullptr;	// 엑스트라 크레딧 하단의 10px 기여자 표기용

	EndingPhase _phase = EndingPhase::Score;
	int32 _slideIndex = 0;		// 0=end01, 1=end02, 2=end06
	float _slideTimer = 0.f;
	bool _fadingOut = false;

	float _extraCreditTimer = 0.f;
};
