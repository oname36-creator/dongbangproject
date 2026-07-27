#pragma once

#include "Singleton.h"


// 텍스처 로드&관리
class ResourceManager : public Singleton<ResourceManager>
{
	// Singleton 객체를 '친구'로 선언해서 private 접근 가능하게 열어준다.
	friend Singleton<ResourceManager>;

public:
	void Init(HWND hwnd, fs::path directory);
	void Cleanup();

	void LoadTexture(wstring key, wstring texturePath, int32 transparent, 
					 int32 row = 1, int32 col = 1, float dur = 0);
	class Texture* GetTexture(wstring key);

private:
	// 아무나 생성못하게 생성자/소멸자를 숨기자
	ResourceManager() = default;
	~ResourceManager() = default;

private:
	// 모든 texture 리소스 폴더내에 있을꺼라, 루트 디렉터리 정보를 미리 만들어두자.
	fs::path _resourcePath;

	// 키 : path
	// Texture*
	unordered_map<wstring, Texture*> _textures;

};

