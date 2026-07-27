#include "pch.h"
#include "TimeManager.h"

// static 변수 초기화
int32 TimeManager::TimerIdGenerator = 0;


void TimeManager::Init()
{
	::QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&_frequency));
	::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&_prevCount)); // CPU 클럭
}

void TimeManager::Update()
{
	uint64 currentCount;
	::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentCount));

	_deltaTime = (currentCount - _prevCount) / static_cast<float>(_frequency);
	_prevCount = currentCount;

	// 프레임율(FPS 계산을 위해)
	_frameCount++;
	_frameTime += _deltaTime;	// 시간을 누적

	// 초당 프레임률 계산
	if (_frameTime >= 1.0f)
	{
		_fps = _frameCount;
		_frameTime -= 1.0f;
		_frameCount = 0;
	}

	// 타이머 업데이트
	// 타이머 호출
	for (auto& iter : _timers)
	{
		iter.Update(_deltaTime);
	}

	// 제거 리스트
	// set<int32> _removeTimers;
	// 타이머의 고유한 Id 값을 가지고 있다.
	// set에 추가된 Id의 타이머를 모두 제거한다.
	_timers.erase(std::remove_if(_timers.begin(), _timers.end(),
		[&](const Timer& timer) {
			return _removeTimers.find(timer.GetId()) != _removeTimers.end();
		}), _timers.end());
	_removeTimers.clear();

	// 추가 리스트
	// 이번 프레임에 추가되어야 하는 타이머들
	_timers.insert(_timers.end(), _addTimers.begin(), _addTimers.end());
	_addTimers.clear();
}

// 타이머는 매프레임 Update 로직 호출중이라, 
// 안전하게 모든 Update로직이 끝난후에 timer list 에 넣는다.
int32 TimeManager::AddTimer(TimerFunc func, float interval, bool loop)
{
	int32 id = TimerIdGenerator++;
	Timer timer(id, loop, func, interval);
	_addTimers.push_back(timer);

	return id;
}

// 타이머는 매프레임 Update 로직 호출중이라, 
// 안전하게 모든 Update로직이 끝난후에 timer list 에서 제거한다.
void TimeManager::Remove(int32 id)
{
	_removeTimers.insert(id);
}

//----------------------------------
// 타이머 객체
//----------------------------------
void Timer::Update(float deltaTime)
{
	_sumTime += deltaTime;

	// 알람이 울릴 시간만큼 충분히 지낫다.
	if (_sumTime >= _interval)
	{
		// 함수 포인터 호출
		_func();

		if (_loop)
		{
			_sumTime -= _interval;
		}
	}
}

bool Timer::IsExpired()
{
	// 타이머의 누적시간이 알람 울려야하는 시간보다 넘어섯다면 만료된것
	return (_sumTime >= _interval);
}
