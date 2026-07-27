#pragma once

#include "Singleton.h"

// 타이머를 위한 함수포인터 저장용
using TimerFunc = std::function<void()>;

class Timer
{
public:
	Timer(int32 id, bool loop, TimerFunc func, float interval) : _id(id), _loop(loop), _func(func), _interval(interval) {}
	void Update(float deltaTime);
	bool IsExpired();
	int32 GetId() const { return _id; }
private:
	int32 _id = 0;
	bool _loop = false;
	TimerFunc _func;
	float _sumTime = 0;     // 지금까지 경과시간 0..
	float _interval = 0;	// 알람 울릴 시간. 2초
};

class TimeManager : public Singleton<TimeManager>
{
	// Singleton 객체를 '친구'로 선언해서 private 접근 가능하게 열어준다.
	friend Singleton<TimeManager>;

public:
	void Init();
	void Update();

	uint32 GetFPS() { return _fps; }

	// 한프레임 지난후 경과시간
	float GetDT() { return _deltaTime; }

	// 타이머
	int32 AddTimer(TimerFunc func, float interval, bool loop = false);
	void Remove(int32 id);


private:
	// 아무나 생성못하게 생성자/소멸자를 숨기자
	TimeManager() = default;
	~TimeManager() = default;

	uint64 _frequency = 0;
	uint64 _prevCount = 0;
	float _deltaTime = 0.f;

	// 내가 최대 몇프레임 그림을 그리는 게임을 만들고 싶은데
	// 프레임 제한
	uint32 _frameCount = 0;
	float _frameTime = 0.f;
	uint32 _fps = 0;

	// 타이머를 위한 객체
	static int32 TimerIdGenerator;

	// 활성화되어있는 모든 타이머를 관리하는 자료구조
	vector<Timer>	_timers;

	// 지연 리스트로 타이머 관리
	vector<Timer>	_addTimers;
	set<int32>		_removeTimers;
};

