#include "pch.h"
#include "Effect.h"
#include "SpriteRenderer.h"

void Effect::Init(wstring textureKey)
{
	// Effect는 Sprite Animation 으로 재생한다.
	//_renderer = new SpriteAnimRenderer();
	SpriteAnimRenderer* renderer = AddComponent<SpriteAnimRenderer>();
	renderer->Init(textureKey);

	// 미리 캐싱해둔다.
	_renderer = renderer;
}

void Effect::Update(float deltaTime)
{
	Super::Update(deltaTime);

	if (_renderer)
	{
		//_renderer->Update(deltaTime);

		// 방식 1) 풀링 방식으로 조건을 체크
		// 방식 2) 함수 포인터. 콜백 받는 방식(event, delegate)
		// 2번 방식이 더 좋지만, 지금은 기존 구조대로 1번 방식으로 한다.
		if (_renderer->IsEnd())
		{
			// 재생시간 끝났으면 삭제
			Destroy();
		}
	}
}

void Effect::Render(HDC hdc)
{
	Super::Render(hdc);

	//if (_renderer)
	//{
	//	_renderer->Render(hdc, GetPos());
	//}
}
