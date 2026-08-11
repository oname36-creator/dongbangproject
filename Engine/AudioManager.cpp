#include "pch.h"
#include "AudioManager.h"

void AudioManager::Init(fs::path directory)
{
	_resourcePath = directory;

	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	XAudio2Create(&_engine, 0, XAUDIO2_DEFAULT_PROCESSOR);
	_engine->CreateMasteringVoice(&_masteringVoice);
}

void AudioManager::Cleanup()
{
	// 남아있는 재생 중인 보이스도 여기서(메인 스레드) 정리한다.
	for (VoiceCallback* callback : _activeVoices)
	{
		callback->voice->DestroyVoice();
		delete callback;
	}
	_activeVoices.clear();

	if (_masteringVoice)
	{
		_masteringVoice->DestroyVoice();
		_masteringVoice = nullptr;
	}

	if (_engine)
	{
		_engine->Release();
		_engine = nullptr;
	}

	_sounds.clear();

	CoUninitialize();
}

void AudioManager::Update()
{
	// OnStreamEnd(오디오 스레드)는 finished 표시만 하고, 실제 정리는 여기(메인 스레드)에서 한다.
	for (size_t i = 0; i < _activeVoices.size(); )
	{
		VoiceCallback* callback = _activeVoices[i];
		if (callback->finished)
		{
			callback->voice->DestroyVoice();
			delete callback;

			_activeVoices[i] = _activeVoices.back();
			_activeVoices.pop_back();
		}
		else
		{
			++i;
		}
	}
}

void AudioManager::LoadSound(wstring key, wstring fileName)
{
	if (_sounds.find(key) != _sounds.end())
		return; // 이미 로드됨

	fs::path fullPath = _resourcePath / fileName;

	ifstream file(fullPath, std::ios::binary);
	if (!file.is_open())
	{
		MessageBox(nullptr, (L"Failed to open sound file: " + fullPath.wstring()).c_str(), L"Error", MB_OK);
		return;
	}

	char chunkId[4];
	uint32 chunkSize = 0;

	// RIFF 헤더: "RIFF" + 전체크기 + "WAVE"
	file.read(chunkId, 4);
	file.read((char*)&chunkSize, 4);
	file.read(chunkId, 4);

	SoundClip clip;
	bool hasFmt = false;
	bool hasData = false;

	// 이후 "fmt "/"data" 청크가 순서대로 나온다. 필요없는 청크는 건너뛴다.
	while (file.read(chunkId, 4))
	{
		file.read((char*)&chunkSize, 4);

		if (strncmp(chunkId, "fmt ", 4) == 0)
		{
			// WAVEFORMATEXTENSIBLE 등 더 큰 포맷일 수 있으므로 구조체 크기만큼만 읽는다.
			uint32 readSize = min(chunkSize, (uint32)sizeof(WAVEFORMATEX));
			file.read((char*)&clip.format, readSize);
			if (chunkSize > readSize)
				file.seekg(chunkSize - readSize, std::ios::cur);
			hasFmt = true;
		}
		else if (strncmp(chunkId, "data", 4) == 0)
		{
			clip.data.resize(chunkSize);
			file.read((char*)clip.data.data(), chunkSize);
			hasData = true;
		}
		else
		{
			file.seekg(chunkSize, std::ios::cur);
		}
	}

	if (!hasFmt || !hasData)
	{
		MessageBox(nullptr, (L"Invalid WAV file: " + fullPath.wstring()).c_str(), L"Error", MB_OK);
		return;
	}

	_sounds.insert(make_pair(key, std::move(clip)));
}

void AudioManager::Play(wstring key)
{
	auto find = _sounds.find(key);
	if (find == _sounds.end())
		return;

	const SoundClip& clip = find->second;

	VoiceCallback* callback = new VoiceCallback();

	IXAudio2SourceVoice* voice = nullptr;
	_engine->CreateSourceVoice(&voice, &clip.format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, callback);
	if (voice == nullptr)
	{
		delete callback;
		return;
	}
	callback->voice = voice;
	_activeVoices.push_back(callback);

	XAUDIO2_BUFFER buffer = {};
	buffer.AudioBytes = (UINT32)clip.data.size();
	buffer.pAudioData = clip.data.data();
	buffer.Flags = XAUDIO2_END_OF_STREAM;

	voice->SubmitSourceBuffer(&buffer);
	voice->Start();
}
