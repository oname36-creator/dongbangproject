#pragma once

#include "Singleton.h"

// 현재 씬(Scene*)을 들고 있다가, 프레임 경계에서 안전하게 다음 씬으로 갈아끼워주는 매니저.
//
// Update() 도중 현재 씬을 바로 delete하면, delete된 객체의 Update()가 여전히
// 콜스택에 남아 있는 상태라 크래시한다. 그래서 ChangeScene()은 전환을 '예약'만 하고,
// 실제 교체는 다음 Update() 맨 앞에서 처리한다.
// (GameScene이 Actor를 _reservedAdd/_reservedRemove로 지연 처리하는 것과 같은 이유)
class SceneManager : public Singleton<SceneManager>
{
	friend Singleton<SceneManager>;

public:
	void Update(float deltaTime);
	void Render(HDC hdc);

	// 씬 전환 예약. 실제 교체는 다음 Update() 시작 시점에 일어난다.
	void ChangeScene(class Scene* newScene);

	class Scene* GetCurrentScene() const { return _curScene; }

	void Cleanup();
private:
	SceneManager() = default;
	~SceneManager() = default;

private:
	class Scene* _curScene = nullptr;
	class Scene* _nextScene = nullptr;
};
