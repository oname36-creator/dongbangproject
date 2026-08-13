#include "pch.h"
#include "SaveManager.h"

void SaveManager::Init(fs::path directory)
{
	fs::path saveDir = directory / L"Save";
	fs::create_directories(saveDir);
	_saveFilePath = saveDir / L"save.json";

	load();
}

void SaveManager::UnlockExtra()
{
	if (_extraUnlocked)
		return;

	_extraUnlocked = true;
	save();
}

void SaveManager::load()
{
	std::ifstream file(_saveFilePath);
	if (!file.is_open())
		return;	// 저장 파일이 없으면 기본값(잠김) 그대로 사용

	json data = json::parse(file, nullptr, false);
	if (data.is_discarded())
		return;

	if (data.contains("extraUnlocked"))
	{
		_extraUnlocked = data["extraUnlocked"].get<bool>();
	}
}

void SaveManager::save()
{
	json data;
	data["extraUnlocked"] = _extraUnlocked;

	std::ofstream file(_saveFilePath);
	if (file.is_open())
	{
		file << data.dump(2);
	}
}
