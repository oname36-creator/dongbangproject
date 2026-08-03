#include "pch.h"
#include "CollisionManager.h"
#include "Game.h"
#include "GameScene.h"
#include "Actor.h"
#include "ColliderCircle.h"
#include "InputManager.h"

void CollisionManager::Init()
{
	// 여긴 충돌체크 필요없는 케이스를 테이블화
	// "돌+나무", "나무+돌"
	setIgnoreMask(ActorType::Enemy, ActorType::EnemyBullet);
	setIgnoreMask(ActorType::Player, ActorType::PlayerBullet);
	setIgnoreMask(ActorType::EnemyBullet, ActorType::PlayerBullet);

	/* 위쪽 코드처럼 함수로 분리시, 간결해진다.
	IGNORE_MASK[(int32)ActorType::Enemy][(int32)ActorType::Enemy] = true;
	IGNORE_MASK[(int32)ActorType::Player][(int32)ActorType::Player] = true;

	IGNORE_MASK[(int32)ActorType::Enemy][(int32)ActorType::EnemyBullet] = true;
	IGNORE_MASK[(int32)ActorType::EnemyBullet][(int32)ActorType::Enemy] = true;

	IGNORE_MASK[(int32)ActorType::Player][(int32)ActorType::PlayerBullet] = true;
	IGNORE_MASK[(int32)ActorType::PlayerBullet][(int32)ActorType::Player] = true;

	IGNORE_MASK[(int32)ActorType::EnemyBullet][(int32)ActorType::PlayerBullet] = true;
	IGNORE_MASK[(int32)ActorType::PlayerBullet][(int32)ActorType::EnemyBullet] = true;
	*/
}

void CollisionManager::Update()
{
	// 현재 씬이 GameScene이 아니면(Title/Result/Ending 등) 그리드 정보를 물어볼 대상이 없다.
	if (Game::GetInstance().GetScene() == nullptr)
		return;

	// 현재 상태에 대한 충돌체크만 수행해서 결과를 저장
	_curr.clear();

	// 충돌체크가 필요한 Actor는 전부다 비교해서 충돌체크를 수행한다.
	// 일단, 모든 녀석들을 다 순회하면서 체크한다.
	// [0] : player, [1] enemy, [2] p.bullet [3] e.bullet ..
	// [0]<->[1], [2], [3] 전부 비교

	// _collisionCheckList : 리스트에 존재한다는건, 충돌체크를 실행해야할 '주체'
	// 현재 : 내 비행기, 적 비행기, 내 총알, 적 총알
	// Grid 방식 : 내 비행기, 내 총알, 

	// 주체가 되는 녀석들만 순회
	for (auto actor : _collisionCheckList)
	{
		// 어떤 대상과 충돌체크를 해야하냐면, 내가 있는 셀과 인접한 셀만 충돌체크 수행
		checkCollision(actor);
	}


	/*
	// 렌더링 순서를 위해서, 미리 목록화된 리스트를 가져와서 충돌체크 수행
	auto bulletList = Game::GetInstance().GetScene()->GetRenderList(RenderLayer::Bullet);
	auto playerList = Game::GetInstance().GetScene()->GetRenderList(RenderLayer::Player);
	auto enemyList = Game::GetInstance().GetScene()->GetRenderList(RenderLayer::Enemy);
	auto itemList = Game::GetInstance().GetScene()->GetRenderList(RenderLayer::Item);
	auto itemList = Game::GetInstance().GetScene()->GetRenderList(RenderLayer::Item);

	// 누구 vs 누구 충돌체크해야하는지 여부만 다를뿐. -> Table
	// 충돌체크를 수행하고, state 갱신하는 로직은 다 똑같다.
	// OnEnter, OnExit
	for (itemList)
	{
		// 플레이어와 충돌체크...
	}

	// 내 총알 vs 적 비행기
	{
		// O( N * M ) : N 내총알, M 적 비행기
		for (auto bullet : bulletList)
		{
			// 총알의 충돌체가 없다면, 무시
			if (bullet->GetCollider() == nullptr)
				continue;

			// 적비행기 총알?? player 랑 충돌


			for (auto enemy : enemyList)
			{
				// 적의 충돌체가 없다면, 무시
				if (enemy->GetCollider() == nullptr)
					continue;

				// 충돌체크 검사
				if (bullet->GetCollider()->CheckCollision(enemy->GetCollider()))
				{
					// 충돌되었다면, 양쪽 Actor에 누구와 충돌했는지 알려준다.
					// Enter, Stay, Exit

					addOverlapState(bullet, enemy);
					
					//bullet->OnHit(enemy);
					//enemy->OnHit(bullet);
				}
			}
		}
	}

	// 내 비행기 vs 적 비행기
	if (playerList.size())
	{
		Actor* player = playerList[0];
		for (auto enemy : enemyList)
		{
			// 적의 충돌체가 없다면, 무시
			if (enemy->GetCollider() == nullptr)
				continue;

			// 충돌체크 검사
			if (player->GetCollider()->CheckCollision(enemy->GetCollider()))
			{
				// 충돌되었다면, 양쪽 Actor에 누구와 충돌했는지 알려준다.
				addOverlapState(player, enemy);

				//player->OnHit(enemy);
				//enemy->OnHit(player);
			}
		}
	}
	*/


	// 현재 프레임에 충돌체크가 필요한 상태 체크 완료
	// Exit 
	for (const auto& iter : _prev)
	{
		// 이전에는 있었는데, 현재는 없다.
		// Exit
		if (_curr.contains(iter) == false)
		{
			// pair<Actor*, Actor*>
			// 양방향으로 Exit 함수를 호출해준다.
			iter.first->OnExit(iter.second);
			iter.second->OnExit(iter.first);
		}
	}

	// curr -> prev
	_prev = _curr;	// 이제부터 curr 상태가 prev 상태로 변경.
	//swap(_prev, _curr);

	// 디버깅 정보 토글
	if (InputManager::GetInstance().GetButtonDown(KeyType::F1))
	{
		//if (_drawDebug)
		//{
		//	_drawDebug = false;
		//}
		//else
		//{
		//	_drawDebug = true;
		//}

		_drawDebug = !_drawDebug;
	}
}

void CollisionManager::Render(HDC hdc)
{
	if (_drawDebug)
	{
		// 그리드 라인 보기
		drawGridLine(hdc);

		// 디버깅을 위한 충돌체 상태 보기
		for (auto actor : _collisionCheckList)
		{
			actor->GetCollider()->Render(hdc, actor->GetPos());
		}
	}
}

void CollisionManager::AddActor(Actor* actor)
{
	// 적비행기, 적총알은 등록(X)
	if (actor->GetCollider() && actor->GetCollider()->CheckCell())
	{
		// 충돌체크가 필요한 객체 추가
		_collisionCheckList.push_back(actor);
	}
}

void CollisionManager::RemoveActor(Actor* actor)
{
	// 충돌체크가 필요한 객체에서 제거
	std::erase_if(_collisionCheckList, [actor](const Actor* iter) 
		{
			return iter == actor;
		});


	// 제거해야할 대상을 걸러주는 람다식
	auto checkActor = [actor](const std::pair<Actor*, Actor*>& pair)
		{
			if (pair.first == actor || pair.second == actor)
				return true;
			return false;
		};

	// 전체순회해도 비용이 크지 않다.
	// 겹쳐져있는 대상만 set 에 추가될꺼에요.
	std::erase_if(_prev, checkActor);
	std::erase_if(_curr, checkActor);
}

void CollisionManager::addOverlapState(Actor* actor1, Actor* actor2)
{
	//  항상 원하는 순서대로 정렬해서 key 를 만들자.
	// "돌"+"나무" => "돌"+"나무"
	// "나무"+"돌" => "돌"+"나무"
	// * point : 일종의 주소값 0x001 < 0x002 (8byte)
	// actor1 : 0x001
	// actor2 : 0x002
	// key -> pair(0x001, 0x002)

	// actor1 : 0x002
	// actor2 : 0x001
	// key -> pair(actor2, actor1)

	auto pair = (actor1 < actor2) ? make_pair(actor1, actor2) : make_pair(actor2, actor1);

	// 현재 프레임에 충돌상태 체크 됨
	bool insert = _curr.insert(pair).second;	// second : true, 중복된 키를 추가했으면, second : false
	bool prev = _prev.contains(pair); // 이전 프레임에 key 조합이 있었는지 확인

	if (insert == true && prev == false)
	{
		actor1->OnEnter(actor2);
		actor2->OnEnter(actor1);
	}
	// Stay 해보고 싶으면
	// 현재 insert == true, 이전 : true

}

void CollisionManager::setIgnoreMask(ActorType A, ActorType B)
{
	// 항상 양방향으로 관리
	IGNORE_MASK[(int32)A][(int32)B] = true;
	IGNORE_MASK[(int32)A][(int32)A] = true;

	IGNORE_MASK[(int32)B][(int32)B] = true;
	IGNORE_MASK[(int32)B][(int32)A] = true;
}

void CollisionManager::checkCollision(Actor* actor)
{
	int32 gridSize = Game::GetInstance().GetScene()->GetGridSize();

	// x, +- 1
	// y, +- 1
	Cell cell = Cell::ConvertToCell(actor->GetPos(), gridSize);

	// 인접한 셀을 순회한다. 9번
	for (int i = -1; i <= 1; ++i)
	{
		for (int j = -1; j <= 1; ++j)
		{
			Cell checkCell{ cell.iX + i, cell.iY + j };

			// 인접한 그리드 셀이 관리하고 있는 Actor를 전체다 순회하면서 충돌체크를 수행
			const GridInfo& gridInfo = Game::GetInstance().GetScene()->GetGridInfo(checkCell);
			for (const auto& otherActor : gridInfo.actors)
			{
				// 같은 녀석은 건너띈다.
				if (actor == otherActor)
					continue;

				// 충돌체크를 안해도 되는 녀석은 건너띈다.
				if (IGNORE_MASK[(int32)actor->GetActorType()][(int32)otherActor->GetActorType()] == true)
					continue;

				// 충돌체크 검사
				if (actor->GetCollider()->CheckCollision(otherActor->GetCollider()))
				{
					// 충돌되었다면, 양쪽 Actor에 누구와 충돌했는지 알려준다.
					// Enter, Stay, Exit
					addOverlapState(actor, otherActor);
					//bullet->OnHit(enemy);
					//enemy->OnHit(bullet);
				}
			}
		}
	}
	
}

void CollisionManager::drawGridLine(HDC hdc)
{
	int32 gridSize = Game::GetInstance().GetScene()->GetGridSize();

	// 빨간색 그리드 배경 선
	{
		// 화면 크기와 그리드 크기 설정
		int32 width = GWinSizeX;
		int32 height = GWinSizeY;

		// 빨간색 펜 생성
		HPEN redPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
		HPEN oldPen = (HPEN)SelectObject(hdc, redPen);

		// 가로선 그리기
		for (int y = 0; y <= height; y += gridSize)
		{
			MoveToEx(hdc, 0, y, nullptr); // 시작점 설정
			LineTo(hdc, width, y);        // 끝점까지 선 그리기
		}

		// 세로선 그리기
		for (int x = 0; x <= width; x += gridSize)
		{
			MoveToEx(hdc, x, 0, nullptr); // 시작점 설정
			LineTo(hdc, x, height);       // 끝점까지 선 그리기
		}

		// 이전 펜 복원 및 새 펜 삭제
		SelectObject(hdc, oldPen);
		DeleteObject(redPen);
	}

	// 충돌 체크가 필요한 선만 그리기 : 진한 청록색
	{
		// 펜 생성
		HPEN myPen = CreatePen(PS_SOLID, 3, RGB(0, 255, 255));
		HPEN oldPen = (HPEN)SelectObject(hdc, myPen);

		for (auto actor : _collisionCheckList)
		{
			const Cell& cell = Cell::ConvertToCell(actor->GetPos(), gridSize);

			// 인접한 셀 모두 표시
			for (int32 i = -1; i < 2; ++i)
			{
				for (int32 j = -1; j < 2; ++j)
				{
					Cell checkCell{ cell.iX + i, cell.iY + j };

					// 사각형 그리기
					int32 x = checkCell.iX * gridSize;
					int32 y = checkCell.iY * gridSize;

					{
						MoveToEx(hdc, x, y, nullptr);
						LineTo(hdc, x + gridSize, y);
					}
					{
						MoveToEx(hdc, x + gridSize, y, nullptr);
						LineTo(hdc, x + gridSize, y + gridSize);
					}
					{
						MoveToEx(hdc, x + gridSize, y + gridSize, nullptr);
						LineTo(hdc, x, y + gridSize);
					}
					{
						MoveToEx(hdc, x, y + gridSize, nullptr);
						LineTo(hdc, x, y);
					}
				}
			}
		}

		// 이전 펜 복원 및 새 펜 삭제
		SelectObject(hdc, oldPen);
		DeleteObject(myPen);
	}
}
