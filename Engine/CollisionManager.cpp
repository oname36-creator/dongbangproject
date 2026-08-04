#include "pch.h"
#include "CollisionManager.h"
#include "Game.h"
#include "GameScene.h"
#include "Actor.h"
#include "ColliderCircle.h"
#include "InputManager.h"

void CollisionManager::Init()
{
	setIgnoreMask(ActorType::Enemy, ActorType::EnemyBullet);
	setIgnoreMask(ActorType::Player, ActorType::PlayerBullet);
	setIgnoreMask(ActorType::EnemyBullet, ActorType::PlayerBullet);
	setIgnoreMask(ActorType::Item, ActorType::Enemy);
	setIgnoreMask(ActorType::Item, ActorType::Boss);
	setIgnoreMask(ActorType::Item, ActorType::EnemyBullet);
	setIgnoreMask(ActorType::Item, ActorType::PlayerBullet);
}

void CollisionManager::Update()
{
	// 현재 씬이 GameScene이 아니면(Title/Result/Ending 등) 그리드 정보를 물어볼 대상이 없다.
	if (Game::GetInstance().GetScene() == nullptr)
		return;

	_curr.clear();

	// 주체가 되는 녀석들만 순회하면서, 인접한 셀만 충돌체크 수행
	for (auto actor : _collisionCheckList)
	{
		checkCollision(actor);
	}

	// 이전에는 겹쳐 있었는데 현재는 아닌 쌍 -> Exit
	for (const auto& iter : _prev)
	{
		if (_curr.contains(iter) == false)
		{
			iter.first->OnExit(iter.second);
			iter.second->OnExit(iter.first);
		}
	}

	_prev = _curr;

	// 디버깅 정보 토글
	if (InputManager::GetInstance().GetButtonDown(KeyType::F1))
	{
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
	if (actor->GetCollider() && actor->GetCollider()->CheckCell())
	{
		_collisionCheckList.push_back(actor);
	}
}

void CollisionManager::RemoveActor(Actor* actor)
{
	std::erase_if(_collisionCheckList, [actor](const Actor* iter)
		{
			return iter == actor;
		});

	auto checkActor = [actor](const std::pair<Actor*, Actor*>& pair)
		{
			if (pair.first == actor || pair.second == actor)
				return true;
			return false;
		};

	// 전체순회해도 비용이 크지 않다. 겹쳐져있는 대상만 set 에 들어있기 때문.
	std::erase_if(_prev, checkActor);
	std::erase_if(_curr, checkActor);
}

void CollisionManager::addOverlapState(Actor* actor1, Actor* actor2)
{
	// 항상 원하는 순서대로 정렬해서 key를 만들어야, (A,B)와 (B,A)가 같은 쌍으로 취급된다.
	auto pair = (actor1 < actor2) ? make_pair(actor1, actor2) : make_pair(actor2, actor1);

	bool insert = _curr.insert(pair).second;	// 중복된 키를 추가했으면 false
	bool prev = _prev.contains(pair);

	if (insert == true && prev == false)
	{
		actor1->OnEnter(actor2);
		actor2->OnEnter(actor1);
	}
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

	Cell cell = Cell::ConvertToCell(actor->GetPos(), gridSize);

	// 인접한 셀을 순회한다 (3x3, 총 9개)
	for (int i = -1; i <= 1; ++i)
	{
		for (int j = -1; j <= 1; ++j)
		{
			Cell checkCell{ cell.iX + i, cell.iY + j };

			const GridInfo& gridInfo = Game::GetInstance().GetScene()->GetGridInfo(checkCell);
			for (const auto& otherActor : gridInfo.actors)
			{
				if (actor == otherActor)
					continue;

				if (IGNORE_MASK[(int32)actor->GetActorType()][(int32)otherActor->GetActorType()] == true)
					continue;

				if (actor->GetCollider()->CheckCollision(otherActor->GetCollider()))
				{
					addOverlapState(actor, otherActor);
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
		int32 width = GWinSizeX;
		int32 height = GWinSizeY;

		HPEN redPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
		HPEN oldPen = (HPEN)SelectObject(hdc, redPen);

		for (int y = 0; y <= height; y += gridSize)
		{
			MoveToEx(hdc, 0, y, nullptr);
			LineTo(hdc, width, y);
		}

		for (int x = 0; x <= width; x += gridSize)
		{
			MoveToEx(hdc, x, 0, nullptr);
			LineTo(hdc, x, height);
		}

		SelectObject(hdc, oldPen);
		DeleteObject(redPen);
	}

	// 충돌 체크가 필요한 셀만 표시: 진한 청록색
	{
		HPEN myPen = CreatePen(PS_SOLID, 3, RGB(0, 255, 255));
		HPEN oldPen = (HPEN)SelectObject(hdc, myPen);

		for (auto actor : _collisionCheckList)
		{
			const Cell& cell = Cell::ConvertToCell(actor->GetPos(), gridSize);

			for (int32 i = -1; i < 2; ++i)
			{
				for (int32 j = -1; j < 2; ++j)
				{
					Cell checkCell{ cell.iX + i, cell.iY + j };

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

		SelectObject(hdc, oldPen);
		DeleteObject(myPen);
	}
}
