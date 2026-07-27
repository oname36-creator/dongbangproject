#include "pch.h"
#include "ResourceManager.h"
#include "Texture.h"

void ResourceManager::Init(HWND hwnd, fs::path directory)
{
	_resourcePath = directory;
}

void ResourceManager::Cleanup()
{
	// 매니저가 생성했던 모든 텍스처를 해제해준다.
	for (auto iter : _textures)
	{
		// 실제 메모리 해제
		delete iter.second; // value : texture 
	}

	// 자료구조도 싹 비우고
	_textures.clear();
}

void ResourceManager::LoadTexture(wstring key, wstring texturePath, int32 transparent,
								  int32 row, int32 col, float dur)
{
	if (GetTexture(key) != nullptr)
	{
		// 이미 추가된 텍스처라서 무시
		return;
	}

	fs::path fullPath = _resourcePath / texturePath;

	// 텍스처 생성을 담당
	Texture* texture = new Texture(); 
	texture->Load(fullPath, transparent, row, col, dur);

	// 생성된 텍스터를 리소스 매니저가 관리 N 개 관리
	_textures.insert(make_pair(key, texture));
}

Texture* ResourceManager::GetTexture(wstring key)
{
	auto find = _textures.find(key);
	if (find != _textures.end())
		return find->second;

	return nullptr;
}
