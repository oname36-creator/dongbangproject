#pragma once

#include "Singleton.h"

class UIManager : public Singleton<UIManager>
{
	friend Singleton<UIManager>;

public:
	void Init();
	void Update(float deltaTime);
	void Render(HDC hdc);

private:
	UIManager() = default;
	~UIManager() = default;

private:
	class Texture* _hpTexture = nullptr;
	HFONT _logoFont = nullptr;
	HFONT _statFont = nullptr;	// Score/Life/Bomb/Power, Phase 표시용(로고보다 작은 크기)
};
