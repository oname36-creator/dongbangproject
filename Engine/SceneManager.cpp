#include "pch.h"
#include "SceneManager.h"
#include "Scene.h"

void SceneManager::Update(float deltaTime)
{
	// 예약된 씬 전환이 있으면, 이번 프레임 로직을 돌리기 전에 먼저 교체한다.
	if (_nextScene)
	{
		if (_curScene)
		{
			_curScene->Cleanup();
			delete _curScene;
		}

		_curScene = _nextScene;
		_nextScene = nullptr;

		_curScene->Init();
	}

	if (_curScene)
	{
		_curScene->Update(deltaTime);
	}
}

void SceneManager::Render(HDC hdc)
{
	if (_curScene)
	{
		_curScene->Render(hdc);
	}
}

void SceneManager::ChangeScene(Scene* newScene)
{
	_nextScene = newScene;
}

void SceneManager::Cleanup()
{
	if(_curScene)
	{
		_curScene->Cleanup();
		delete _curScene;
		_curScene = nullptr;
	}
}