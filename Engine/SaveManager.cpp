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

void SaveManager::SetBGMVolume(float volume)
{
	_bgmVolume = std::clamp(volume, 0.f, 1.f);
	save();
}

void SaveManager::SetSFXVolume(float volume)
{
	_sfxVolume = std::clamp(volume, 0.f, 1.f);
	save();
}

void SaveManager::load()
{
	std::ifstream file(_saveFilePath);
	if (!file.is_open())
		return;	// 저장 파일이 없으면 기본값(잠김, 볼륨 100%) 그대로 사용

	json data = json::parse(file, nullptr, false);
	if (data.is_discarded())
		return;

	if (data.contains("extraUnlocked"))
	{
		_extraUnlocked = data["extraUnlocked"].get<bool>();
	}
	if (data.contains("bgmVolume"))
	{
		_bgmVolume = data["bgmVolume"].get<float>();
	}
	if (data.contains("sfxVolume"))
	{
		_sfxVolume = data["sfxVolume"].get<float>();
	}
}

void SaveManager::save()
{
	json data;
	data["extraUnlocked"] = _extraUnlocked;
	data["bgmVolume"] = _bgmVolume;
	data["sfxVolume"] = _sfxVolume;

	std::ofstream file(_saveFilePath);
	if (file.is_open())
	{
		file << data.dump(2);
	}
}
