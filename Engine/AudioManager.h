#pragma once

#include "Singleton.h"
#include <xaudio2.h>
#include <atomic>

// WAV 사운드를 로드해서 XAudio2로 재생.
// PlaySound(WinAPI)와 달리 재생마다 독립적인 소스 보이스를 만들기 때문에
// 여러 사운드가 동시에 겹쳐 재생될 수 있다(다중/폴리포닉 재생).
class AudioManager : public Singleton<AudioManager>
{
	friend Singleton<AudioManager>;

public:
	void Init(fs::path directory);
	void Cleanup();
	void Update();

	void LoadSound(wstring key, wstring fileName);
	void Play(wstring key);

private:
	AudioManager() = default;
	~AudioManager() = default;

private:
	struct SoundClip
	{
		WAVEFORMATEX format = {};
		vector<BYTE> data;
	};

	// 재생이 끝나면(OnStreamEnd) finished만 표시해두는 콜백.
	// OnStreamEnd는 XAudio2 내부 오디오 스레드에서 호출되는데,
	// 그 안에서 자신의 DestroyVoice()를 호출하면 안 되기 때문에(자기 자신을 기다리며 크래시)
	// 실제 정리(DestroyVoice + delete)는 AudioManager::Update()가 메인 스레드에서 수행한다.
	class VoiceCallback : public IXAudio2VoiceCallback
	{
	public:
		IXAudio2SourceVoice* voice = nullptr;
		std::atomic<bool> finished = false;

		void OnStreamEnd() noexcept override { finished = true; }
		void OnVoiceProcessingPassStart(UINT32) noexcept override {}
		void OnVoiceProcessingPassEnd() noexcept override {}
		void OnBufferStart(void*) noexcept override {}
		void OnBufferEnd(void*) noexcept override {}
		void OnLoopEnd(void*) noexcept override {}
		void OnVoiceError(void*, HRESULT) noexcept override {}
	};

private:
	fs::path _resourcePath;
	IXAudio2* _engine = nullptr;
	IXAudio2MasteringVoice* _masteringVoice = nullptr;
	unordered_map<wstring, SoundClip> _sounds;

	// 재생 중인 소스 보이스들. 끝난 것은 Update()에서 정리한다.
	vector<VoiceCallback*> _activeVoices;
};
