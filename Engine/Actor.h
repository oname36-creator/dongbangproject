#pragma once

// Scene에 그려지는 모든 객체들은 Actor로부터 파생된다.
class Actor
{
public:
	virtual ~Actor();
	
	void Init();
	virtual void Destroy();

	virtual void Update(float deltaTime);
	virtual void Render(HDC hdc);
	//virtual void OnHit(Actor* other) {}

	// 유니티와 비슷한 충돌 3단계 함수 
	virtual void OnEnter(Actor* other) {}
	virtual void OnStay(Actor* other) {}
	virtual void OnExit(Actor* other) {}

	Vector GetPos() const { return _pos; }
	void SetPos(Vector pos) 
	{
		// 좌표가 바뀔때는 무조건 SetPos() 함수를 통해서 들어온다.
		_pos = pos; 

		// 이전 셀에서 지우고, 새로운 셀에 등록
		//_currCell(3,3)
		//Scene-> 3.3 제거, 4,3 등록.
	}

	bool GetPendingKill() const { return _pendingKill; }

	// 순수 가상함수. 모든 Actor는 반드시 본인이 그려져야하는 Layer 순서를 알려줘야한다.
	virtual RenderLayer GetRenderLayer() = 0;
	virtual ActorType GetActorType() = 0;

	// 충돌체크가 필요하다면
	virtual class ColliderCircle* GetCollider() { return nullptr; }

	// 오브젝트 풀
	class IObjectPool* GetPool() { return _pool; }
	void SetPool(IObjectPool* pool) { _pool = pool; }


	// 초보자는 이런 구조도 괜찮다. 
	//bool CheckCollision();

	template<typename T>
	T* AddComponent()
	{
		T* newComponent = new T();
		_components.push_back(newComponent);

		return newComponent;
	}

	template<typename T>
	T* GetComponent()
	{
		for (auto iter : _components)
		{
			// Component* -> ColliderCircle* ? ImageRenderer*
			if (T* find = dynamic_cast<T*>(iter))
			{
				return find;
			}
		}

		return nullptr;
	}

private:
	Vector _pos;

	// N개 vector, map
	vector<class Component*> _components;

	//int32 _radius;	// 원vs원 충돌체 크기
	//int32 _witdh;
	//int32 _height;

	bool _pendingKill = false;	// 다음 프레임에 삭제될 예정

	// 기록을 해둔다.
	//Cell _currCell;

	// 내가 태어난 곳이 어디지? 출생지를 기록
	// new 태어난경우 : nullptr
	// pool 태어난경우 : 본인 풀의 주소
	//ObjectPool<Bullet> : 템플릿을 정의하는게 아니라, interface 역할의 자료형으로 선언한다.
	IObjectPool* _pool = nullptr;
};

