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
};
