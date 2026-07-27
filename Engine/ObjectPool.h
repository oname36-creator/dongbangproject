#pragma once

// Actor가 본인의 출신지(Pool) 알아야하는데,
// Pool 이 템플릿과 결합되어있어서...
// 템플릿 없는 자료형으로 Pool 자체의 주소가 필요하다.
class IObjectPool
{
public:
	virtual void Return(class Actor* actor) = 0;
};


template<typename T>
class ObjectPool : public IObjectPool
{
public:
	// 초기화
	void Init(int32 size)
	{
		// 메모리 할당+객체를 생성해서 추가
		_buffer.resize(size);
		//_bufferList.push_back(_buffer);

		// 사용가능한 리스트를 만들어준다.
		// 메모리 확보만 미리 한번 해두고, 실제 요소는 필요할때마다 추가
		// push_back할때마다, capacity 다쓰면 메모리 재할당
		_freeList.reserve(size); //capacity만 확보

		for (auto& iter : _buffer)
		{
			// ObjectPool 이 Actor를 생성하면서
			// 풀 출생지를 알려준다.
			Actor* actor = static_cast<Actor*>(&iter);
			actor->SetPool(this);

			// vector에 저장된 원본객체의 주소를 넘겨야해서 & 붙였다.
			// 그리고 freeList는 원본 객체의 주소만 저장해둔다.
			_freeList.push_back(&iter);
		}
	}

	// 꺼내쓰는것
	T* Acquire()
	{
		// 처음에 100개를 만들었는데, 100개를 다썻다.
		// 다쓴 상태에서 주세요. 요청이 오면??
		if (_freeList.empty())
		{
			// Pool을 두배로 늘린다.
			// 어떤 문제가 있을까?
			// vector 자료구조의 특성상... capacity 다쓴상태에서 추가할경우 '메모리 재할당' 
			// 메모리 재할당시 기존의 원본 메모리가 해제되는 문제 발생.
			// 여기저기 흩어져서 원본 객체의 주소값을 저장하고 있는곳이 크래시 날수도 있다.
			//_buffer.insert(100);

			// 해결 1) 스마트 포인터 변경
			// 해결 2) 재할당 하지말던가.
			
			// 해결 2-1) 부족하면 게임 종료.
			// 풀은 충분히 큰 크기로 Init 되었다는 전제 하에 동작 (자동 확장 없음)
			assert(false && "ObjectPool exhausted: increase Init(size)");
			return nullptr;

			// 해결 2-2) 추가로 메모리 블럭 할당
			//vector<Bullet> newBuffer;
			//newBuffer.resize(100);
			//_bufferList.push_back(newBuffer); //[100] [100]
		}
		 
		// vector는 마지막 요소 제거시, 복사비용 X
		// 0x0A
		T* object = _freeList.back();
		_freeList.pop_back();

		return object;
	}

	// 반환하는것
	virtual void Return(Actor* actor) override
	{
		_freeList.push_back((T*)actor);
	}

private:
	// 메모리가 할당되어 있는 원본
	vector<T>	_buffer; // 여기서 [0] 하나 꺼내씀.
								     // 또 필요하면 [1]
									 // 또 필요하면 [2]
									 // [0] 다썻다고 반환했음.
	                                 // 또 필요하면 [0] ? [3]
									 // 
	
	// 오브젝트 풀이 부족하면, 메모리 블럭을 하나 더 만든다.
	//vector<vector<class Bullet>> _bufferList;

	// 원본 중에서 사용가능한 객체들의 주소값 관리
	vector<T*>  _freeList;
};

