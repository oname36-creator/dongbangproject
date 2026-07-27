#pragma once

#include "ObjectPool.h"

//#include "Enemy.h"
//#include "Bullet.h"
// C++17
class Enemy;
class Bullet;

// 게임화면에 등장하는 모든 오브젝트를 관리
//
// TODO(1주차 Day1~2): 이 클래스를 둘로 쪼갤 것
//  현재: Scene 하나가 '씬의 공통 골격'과 '게임 플레이 로직'을 동시에 들고 있다.
//        virtual 함수가 하나도 없어서 다른 씬으로 갈아끼울 수 없다.
//  목표:
//   - Scene        : 추상 베이스. virtual ~Scene(), virtual Init/Cleanup/Update/Render 만 가진다.
//   - GameScene    : 지금 이 파일의 내용 전부(액터 관리/풀/그리드/스폰)를 옮겨 담는다.
//   - TitleScene   : 배경 + "Press Z to Start" + 키 입력 → GameScene으로 전환
//   - ResultScene  : 점수 표시 + 키 입력 → TitleScene으로 전환
//  힌트: Actor가 이미 같은 구조다(Actor.h의 virtual Update/Render를 Player/Enemy가 override).
//        똑같은 방식으로 만들면 된다.
//  주의: 아래 생성자/소멸자 주석에 적힌 이유(ObjectPool<T> 때문에 cpp에 구현) 때문에
//        GameScene의 생성자/소멸자도 반드시 cpp 쪽에 둬야 한다.
class Scene
{
public:
	// vector<T> 풀에서 사용하는 Enemy,Bullet 값자체를 전방선언으로 해결하기 위해
	// Scene의 생성자와 소멸자는 cpp 쪽에 구현을 해야한다.
	Scene();
	~Scene();

	void Init();
	void Cleanup();

	void Update(float deltaTime);
	void Render(HDC hdc);

	// TODO(1주차 Day6~7): GameScene 상태머신을 추가할 것
	//  목표(기획서 3장): Ready → Playing → Boss → Clear / GameOver
	//   - Ready    : 스테이지 이름/조작법을 1~2초 보여준다
	//   - Playing  : 웨이브 스폰과 일반 전투
	//   - Boss     : 보스 UI 활성화 (2주차에 채운다. 1주차엔 상태만 만들어 두면 된다)
	//   - Clear    : 남은 탄 정리 → 점수 집계 → ResultScene 전환
	//   - GameOver : 입력 제한 → 재시작/타이틀 이동
	//  할 일: enum class GameState 를 선언하고 _state 멤버를 둔 뒤,
	//        Update()에서 switch로 분기한다. (1주차엔 switch 골격만 있어도 충분)
	//  힌트: Ready의 '1~2초 대기'는 TimeManager::AddTimer(func, 2.0f, false)로 간단히 된다.
	//  함정: GameOver 상태에서도 Update가 계속 돌면 적이 계속 스폰된다.
	//        상태별로 '무엇을 멈출지'를 반드시 정해라.

	// 씬에서 관리되는 Actor중에 하나 삭제해달라고 요청
	void DeleteActor(class Actor* actor);

	void CreateBullet(Vector pos, BulletType type);
	void CreateEffect(Vector pos);

	// 좌표계 변환해주는 함수
	Vector ConvertWorldToScreen(Vector worldPos);

public:
	const vector<Actor*>& GetRenderList(RenderLayer layer) const;
	const GridInfo& GetGridInfo(const Cell& cell);
	int32 GetGridSize() const { return _gridSize; }
	class Player* GetPlayer() const { return _player; }

private:
	void loadResources();
	void createObjects();

	// actor List / render List 의 동기화를 맞춰주기 위해서, 항상 호출되는 함수
	void registerActor(Actor* actor);
	void removeActor(Actor* actor);

	// TODO(1주차 Day6~7): 랜덤 스폰을 '웨이브 테이블'로 교체할 것
	//  현재: createRandomEnemy()가 2초마다 무작위로 적을 뿌린다 → 스테이지 설계가 불가능하다.
	//  목표(기획서 7장 Stage 1): 시간대별로 정해진 적이 정해진 위치에 나온다.
	//  할 일: { 등장시간, 적 종류, 등장위치, 개수 } 구조체의 배열을 만들고,
	//        스테이지 경과 시간을 누적하며 때가 된 웨이브를 스폰한다.
	//  힌트: 기획서 5장 설계 원칙대로 JSON 같은 외부 데이터로 빼지 마라.
	//        cpp 안의 배열(코드 테이블)로 시작하는 게 이번 달엔 훨씬 빠르고 디버깅도 쉽다.
	void createRandomEnemy();

	void updateGrid(Actor* actor);

private:
	// 씬에 등장하는 모든 객체는 Actor로부터 파생된 클래스다.
	// 모든 클래스를 관리하는 공통 자료구조를 선언
	vector<Actor*> _actors;	 // 여기가 진짜 Update,Render하는 객체들

	// 렌더링 순서를 위한 list
	vector<Actor*> _renderList[(int32)RenderLayer::Count];
	
	// 지연 시스템
	// 이번 프레임에 추가되어야 하는 Actor들
	// add에 두번요청 들어올일이 없을것 같아서, vector 처리
	vector<Actor*>		_reservedAdd;	

	// 제거 요청을 중복처리하지 않기 위해, set 자료구조
	unordered_set<Actor*>	 _reservedRemove;		// vector vs map

	//class Background* _bg = nullptr;
	//class Player* _player = nullptr;
	//vector<class Enemy*> _enemies;
	//vector<class Bullet*> _bullets;

	// 공간 분할 (Grid)
	int32 _gridSize = 50;	// 유동적으로 수정하면 된다.
	int32 _gridCountX = 0;
	int32 _gridCountY = 0;

	// 하나의 그리드에 있는 Actor 관리
	vector<GridInfo> _grid;

	//vector<Actor*> _test1;    //1 : 메모리 연속X  / [0x01][0x02][0x10][][][]
	//vector<Actor> _test2;		//2 : 메모리 연속	//[Actor][Actor][Actor][Actor][Actor][Actor]
	
	// 메모리가 연속적이지 않아서, 2차원 vector (X) -> 1차원 vector (cell(x,y))
	//vector<vector<GridInfo>> _grid;	// 2차원 배열, [y][x]


	// ObjectPool 객체 관리
	// 목적 1 : new/delete 빈번하게 하지 않는다 -> 메모리 단편화 방지
	// 목적 2 : 캐시 히트율 -> 메모리가 연속적이어야 한다
	//vector<class Bullet>	_bulletList; // Bullet [100]; vs Bullet* [100]
	//vector<class Enemy>		_enemyList;
	//vector<class Effect>	_effectList;
	ObjectPool<Bullet> _bulletPool;
	ObjectPool<Enemy>  _enemyPool;

	// TODO(1주차 Day6~7): 점수 시스템 추가 (기획서 9장)
	//  int32 _score = 0; 하나면 시작할 수 있다.
	//  가산 지점은 이미 표시되어 있다 → Enemy::OnEnter()의 "// 점수 증가" 주석 자리.
	//  배점: 소형 적 100 / 중형 1,000 / 보스 페이즈 10,000 (기획서 9장 표)
	//  참고: 파일 저장은 지금 하지 마라. 실행 중에만 유지되면 1주차 목표로는 충분하다.

	// 카메라 좌표 추가
	// TODO(1주차 Day5): 좌표계를 화면 고정으로 단순화할 것
	//  현재: '화면보다 큰 월드 + 플레이어를 따라가는 카메라' 구조다.
	//        그런데 Player::move()는 GWinSizeX/Y(화면 크기)로 이동을 제한하고 있어서
	//        월드 좌표와 화면 좌표가 뒤섞여 있다. 이대로 2주차 탄막에 들어가면
	//        '탄이 화면 밖으로 나갔는지' 판정이 계속 헷갈린다.
	//  목표: 종스크롤 STG 표준대로 화면은 고정, 배경만 스크롤.
	//  할 일: 카메라 관련 코드를 지우지 말고, _cameraPos를 화면 중앙에 '고정'해라.
	//        그러면 ConvertWorldToScreen()이 항등 함수가 되어 월드 좌표 = 화면 좌표가 된다.
	//        (코드를 삭제하지 않으므로 되돌리기 쉽고, 그리드 코드도 그대로 동작한다)
	Vector _cameraPos;
	Vector _mapSize;
	class Player* _player = nullptr;
};


