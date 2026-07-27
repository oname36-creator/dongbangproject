#pragma once
#include "Singleton.h"

class CollisionManager : public Singleton<CollisionManager>
{
	// Singleton 객체를 '친구'로 선언해서 private 접근 가능하게 열어준다.
	friend Singleton<CollisionManager>;
public:

	void Init();
	void Update();
	void Render(HDC hdc);

	// 충돌체크가 필요한 녀석들
	void AddActor(class Actor* actor);
	void RemoveActor(class Actor* actor);

private:
	void addOverlapState(class Actor* actor1, class Actor* actor2);
	void setIgnoreMask(ActorType A, ActorType B);

	// actor와 인접한 셀을 훑으면서 충돌체크 수행
	void checkCollision(Actor* actor);
	
	// 디버깅용 라인 그리기
	void drawGridLine(HDC hdc);

	// 아무나 생성못하게 생성자/소멸자를 숨기자
	CollisionManager() = default;
	~CollisionManager() = default;

private:
	bool _drawDebug = false;

	// 돌+나무 = 도끼
	// 나무+돌 = 도끼

	//  항상 원하는 순서대로 정렬해서 key 를 만들자.
	// "돌"+"나무" => "돌"+"나무"
	// "나무"+"돌" => "돌"+"나무"
	//struct CollisionPair
	//{
	//	Actor* src;
	//	Actor* other;
	//};
	// 스마트 포인터
	//shared_ptr<> 객체의 생명주기에 관련하고 싶을떄 -> Scene 의 역할
	//weak_ptr<> 관찰하고 싶을떄 -> CollisionManager에게는 이게 더 어울린다.

	set<std::pair<Actor*, Actor*>> _prev; // 이전에 (충돌된 쌍) 관리
	set<std::pair<Actor*, Actor*>> _curr; // 이전에 (충돌된 쌍) 관리

	// 충돌체크 해야하는 모든 Actor
	// 디폴트로 무조건 충돌체크 수행,
	// ( ActorType vs ActorType ) 옵션에 따라서 충돌체크 무시
	vector<Actor*> _collisionCheckList; // (E,P)bullet, player, enemy

	// ignore[0][0] = false; // 충돌체크 off
	// ignore[0][1] = true;	 // 충돌체크 on
	bool IGNORE_MASK[(int32)ActorType::Count][(int32)ActorType::Count] = {};


	// bit 체크로 해보는것 연습해봐도 좋습니다.
	// 1byte : 8bit, 기껏해봐야 ActorType수가 많지 않아서 
	//uint8 IGNORE_BIT_MASK[(int32)ActorType::Count];
	// _ _ _ p.buller e.bullet enmey  player
	// _ _ _     _         _      _     _
	// 비트 자리수가 의미하는것 (ActorType)
};

