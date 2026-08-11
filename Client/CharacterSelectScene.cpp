#include "pch.h"
#include "CharacterSelectScene.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "GameScene.h"
#include "ResourceManager.h"
#include "Texture.h"

namespace
{
	constexpr float FLASH_INTERVAL = 0.1f;	
	constexpr float FLASH_DURATION = 0.6f;
}

void CharacterSelectScene::Init()
{
	ResourceManager::GetInstance().LoadTexture(L"CharacterSelectBG", L"CharacterSelectBG.bmp", -1);
	ResourceManager::GetInstance().LoadTexture(L"ReimuPortrait", L"ReimuPortrait.bmp", RGB(255, 0, 255));
	ResourceManager::GetInstance().LoadTexture(L"ReimuPortraitFlash", L"ReimuPortraitFlash.bmp", RGB(255, 0, 255));
}

void CharacterSelectScene::Update(float deltaTime)
{
	if (!_confirmed)
	{
		if (InputManager::GetInstance().GetButtonDown(KeyType::ATTACK))
		{
			_confirmed = true;
		}
		return;
	}

	_flashElapsed += deltaTime;
	if (_flashElapsed >= FLASH_DURATION)
	{
		SceneManager::GetInstance().ChangeScene(new GameScene());
	}
}

void CharacterSelectScene::Render(HDC hdc)
{
	RECT rect{ 0, 0, GWindowSizeX, GWinSizeY };
	FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

	Texture* bg = ResourceManager::GetInstance().GetTexture(L"CharacterSelectBG");
	if (bg)
	{
		// 가로를 화면 폭에 맞추고 세로는 비율 유지
		float destSizeX = static_cast<float>(GWindowSizeX);
		float destSizeY = destSizeX * (bg->GetSizeY() / static_cast<float>(bg->GetSizeX()));
		bg->RenderScreen(hdc, Vector(GWindowSizeX / 2.0f, GWinSizeY / 2.0f), Vector(0, 0), Vector(destSizeX, destSizeY));
	}

	// 확정 후 FLASH_INTERVAL마다 원본/화이트 실루엣을 번갈아 그려 반짝이는 효과를 낸다.
	bool showFlash = _confirmed && (static_cast<int32>(_flashElapsed / FLASH_INTERVAL) % 2 == 1);
	Texture* portrait = ResourceManager::GetInstance().GetTexture(showFlash ? L"ReimuPortraitFlash" : L"ReimuPortrait");
	if (portrait)
	{
		portrait->RenderScreen(hdc, Vector(GWindowSizeX - 150.0f, GWinSizeY / 2.0f));
	}

	if (!_confirmed)
	{
		const wchar_t* guide = L"Press Z to Select";
		::TextOut(hdc, GWindowSizeX / 2 - 350, GWinSizeY - 60, guide, static_cast<int32>(wcslen(guide)));
	}
}
