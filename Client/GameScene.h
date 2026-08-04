#pragma once

#include "Scene.h"
#include "ObjectPool.h"
#include "Item.h"


//#include "Enemy.h"
//#include "Bullet.h"
// C++17
class Enemy;
class Bullet;
class Boss;


// 게임화면에 등장하는 모든 오브젝트를 관리
class GameScene : public Scene
{
public:
	// vector<T> 풀에서 사용하는 Enemy,Bullet 값자체를 전방선언으로 해결하기 위해
	// Scene의 생성자와 소멸자는 cpp 쪽에 구현을 해야한다.
	GameScene();
	~GameScene();

	virtual void Init() override;
	virtual void Cleanup() override;

	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdc) override;



	enum class GameSceneState
	{
		Ready,
		Playing,
		Boss,
		Stage2,
		Stage2Boss,
		Stage3,
		Stage3MidBoss,
		Stage3Boss,
		Clear,
		Continue,
		GameOver
	};
	// WaveEntry/g_waveTable은 GameScene.cpp 상단(파일 스코프)에 정의되어 있다.
	// 크기를 명시하지 않은 배열(WaveEntry[])은 파일 스코프 상수일 때만 초기화 목록으로
	// 크기를 유추할 수 있고, 클래스의 일반 멤버로는 쓸 수 없다.
	struct WaveEntry;
	GameSceneState _state = GameSceneState::Ready;


	// 씬에서 관리되는 Actor중에 하나 삭제해달라고 요청
	void DeleteActor(class Actor* actor);

	void CreateBullet(Vector pos, BulletType type, Vector dir, float speed = 500.f, bool isHoming = false, float turnSpeed = 180.f);
	void FireStraight(Vector pos, BulletType type, Vector dir, float speed = 500.f);
	void FireAimed(Vector pos, BulletType type, Vector targetPos, float speed = 500.f);
	void FireFan(Vector pos,BulletType type,Vector dir,float angleSpread,int32 count,float speed);
	void FireCircle(Vector pos, BulletType type, int32 count, float speed);
	void FireSpiral(Vector pos,BulletType type,int32 count,float speed,float& rotationAngle, float rotationSpeed);
	void FireRandom(Vector pos, BulletType type, int32 count, float speed);
	void FireHoming(Vector pos, BulletType type, Vector dir, float speed = 500.f, float turnSpeed = 180.f);
	void FireGrid(Vector origin, BulletType type, Vector dir, int32 raws, int32 cols, float spacingX, float spacingY, float speed);
	void FireCross(float y, BulletType type, float speed);

	void CreateEffect(Vector pos);
	void ClearEnemyBullets();

	void SpawnWave(const WaveEntry& wave);
	void SpawnItem(Vector pos, ItemKind kind);

	// 좌표계 변환해주는 함수
	Vector ConvertWorldToScreen(Vector worldPos);

public:
	const vector<Actor*>& GetRenderList(RenderLayer layer) const;
	const GridInfo& GetGridInfo(const Cell& cell);
	int32 GetGridSize() const { return _gridSize; }
	class Player* GetPlayer() const { return _player; }
	class Boss* GetBoss() const { return _boss; }
	Actor* FindNearestEnemy(Vector pos);

	
	int32 GetScore() const { return _score;}
	void AddScore(int32 amount) {_score += amount;}
private:
	void loadResources();
	void createObjects();
	void onPlayerDead();	// 플레이어 사망 시 호출: 컨티뉴 가능하면 Continue, 아니면 GameOver로 분기

	// actor List / render List 의 동기화를 맞춰주기 위해서, 항상 호출되는 함수
	void registerActor(Actor* actor);
	void removeActor(Actor* actor);

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
	ObjectPool<Item> _itemPool;

	bool _isPaused = false;

	int32 _score = 0;
	float _stageElapsedTime = 0.f;
	int32 _nextWaveIndex = 0;
	int32 _nextStage2WaveIndex = 0;
	int32 _nextStage3WaveIndex = 0;
	bool _stage3Wave2 = false;	// false: Stage3 전반부 웨이브 재생 중, true: 중간보스 격파 후 후반부 웨이브 재생 중

	// 카메라 좌표: 화면 중앙에 고정 (종스크롤 STG는 화면 고정 + 배경 스크롤 구조)
	// ConvertWorldToScreen()의 offset이 0이 되어 월드 좌표 = 화면 좌표가 된다.
	Vector _cameraPos = {GWinSizeX/2, GWinSizeY/2};
	Vector _mapSize;
	class Player* _player = nullptr;
	class Boss* _boss = nullptr;
	bool _bossSpawned = false;

	// 컨티뉴 시스템: 최대 3번까지, 몇 번째를 선택 중인지(Yes/No)
	int32 _continueCount = 0;
	static constexpr int32 MAX_CONTINUE = 3;
	bool _continueSelectYes = true;
	GameSceneState _stateBeforeDeath = GameSceneState::Playing;	// Continue Yes 선택 시 되돌아갈 상태

	// Stage2 진입 시 텍스처를 교체하기 위해 패럴랙스 배경 두 장을 캐싱해둔다.
	class Background* _bgLayer1 = nullptr;
	class Background* _bgLayer2 = nullptr;
};


