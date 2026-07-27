#include "pch.h"
#include "WorldBG.h"

#include "ResourceManager.h"
#include "Texture.h"
#include "ImageRenderer.h"

void WorldBG::Init()
{
	//_renderer = new ImageRenderer();
	ImageRenderer* renderer = AddComponent<ImageRenderer>();
	renderer->Init(L"World_BG");
	renderer->SetApplyCenter(false);
}

void WorldBG::Update(float deltaTime)
{

}

void WorldBG::Render(HDC hdc)
{
	Super::Render(hdc);
}

Vector WorldBG::GetMapSize()
{
	ImageRenderer* renderer = GetComponent<ImageRenderer>();
	if (renderer)
	{
		return Vector(renderer->GetSizeX(), renderer->GetSizeY());
	}
	return Vector(0, 0);
}
