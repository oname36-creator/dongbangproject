#pragma once

#include "Singleton.h"

// 재실행해도 유지되어야 하는 진행 상황(현재는 엑스트라 스테이지 언락 여부)을 파일로 저장/로드한다.
class SaveManager : public Singleton<SaveManager>
{
	friend Singleton<SaveManager>;

public:
	void Init(fs::path directory);

	bool IsExtraUnlocked() const { return _extraUnlocked; }
	void UnlockExtra();

	float GetBGMVolume() const { return _bgmVolume; }
	float GetSFXVolume() const { return _sfxVolume; }
	void SetBGMVolume(float volume);
	void SetSFXVolume(float volume);

private:
	SaveManager() = default;
	~SaveManager() = default;

	void load();
	void save();

private:
	fs::path _saveFilePath;
	bool _extraUnlocked = false;
	float _bgmVolume = 1.0f;
	float _sfxVolume = 1.0f;
};
