#pragma once

#include "Scene.h"
#include "ObjectPool.h"
#include "Item.h"


//#include "Enemy.h"
//#include "Bullet.h"
// C++17
class Enemy;
class Bullet;
class LaserSegment;
class Boss;
class Laser;
class BossIllusion;


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
	virtual void RenderOverlay(HDC hdc) override;



	enum class GameSceneState
	{
		Ready,
		Playing,
		Boss,
		StageResult,
		Stage2,
		Stage2Boss,
		Stage3,
		Stage3MidBoss,
		Stage3Boss,
		Extra,
		ExtraBoss,
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

	// Laser가 매 프레임 위치를 재계산해서 SetPos()로 갱신할 세그먼트를 하나 꺼내준다.
	// (Bullet과 달리 Laser 쪽에서 포인터를 계속 들고 있어야 해서 반환값을 넘겨준다)
	class LaserSegment* CreateLaserSegment(Vector pos, int32 radius);
	class Laser* CreateLaser(Vector pivot, float angleOffset, float length, int32 segmentCount, int32 segmentRadius);
	class BossIllusion* CreateBossIllusion(Vector pos, wstring key, int32 hp, float shootOffset, wstring fanTextureKey);

	void CreateBullet(Vector pos, BulletType type, Vector dir, float speed = 500.f, bool isHoming = false, float turnSpeed = 180.f, float accel = 0.f,
					   float preStopTime = 0.f, float launchDelay = 0.f, BulletRedirectMode redirectMode = BulletRedirectMode::None,
					   wstring customTextureKey = L"", float colliderSizeOverride = -1.f, bool faceDirection = false, float targetSpeed = -1.f,
					   float lifeTime = -1.f, float fallAccel = 0.f, bool reflectOffWalls = false);
	void FireStraight(Vector pos, BulletType type, Vector dir, float speed = 500.f, wstring customTextureKey = L"", float colliderSizeOverride = -1.f,
					   bool faceDirection = false);
	void FireAimed(Vector pos, BulletType type, Vector targetPos, float speed = 500.f);
	void FireFan(Vector pos, BulletType type, Vector dir, float angleSpread, int32 count, float speed,
				 wstring customTextureKey = L"", float colliderSizeOverride = -1.f, bool faceDirection = false);
	void FireCircle(Vector pos, BulletType type, int32 count, float speed,
					 float preStopTime = 0.f, float launchDelay = 0.f, BulletRedirectMode redirectMode = BulletRedirectMode::None,
					 wstring customTextureKey = L"", float colliderSizeOverride = -1.f, bool faceDirection = false, float startAngle = 0.f);
	void FireSpiral(Vector pos, BulletType type, int32 count, float speed, float& rotationAngle, float rotationSpeed,
					 wstring customTextureKey = L"", float colliderSizeOverride = -1.f, bool faceDirection = false);
	void FireRandom(Vector pos, BulletType type, int32 count, float speed, float accel,
					 float preStopTime = 0.f, float launchDelay = 0.f, BulletRedirectMode redirectMode = BulletRedirectMode::None,
					 wstring customTextureKey = L"", float colliderSizeOverride = -1.f, bool faceDirection = false);
	void FireHoming(Vector pos, BulletType type, Vector dir, float speed = 500.f, float turnSpeed = 180.f);
	void FireGrid(Vector origin, BulletType type, Vector dir, int32 raws, int32 cols, float spacingX, float spacingY, float speed, wstring customTextureKey = L"", float colliderSizeOverride = -1.f);
	void FireCross(float y, BulletType type, float speed);

	void CreateEffect(Vector pos);
	void ClearEnemyBullets();
	void ClearBossIllusions();	// Boss 레이어에 남아있는 분신(BossIllusion)만 골라서 정리 (본체는 건드리지 않음)
	void BombClearBullets();	// 폭탄 전용: 적 탄환을 점수 아이템으로 바꿔서 플레이어에게 자동 회수시킨다
	void ShowBombFace();	// 폭탄 사용 시 화면 왼쪽 아래에 1.5초간 일러스트를 띄운다.
	void ShowFace(wstring textureKey);	// 보스 진입 등, 임의의 일러스트를 같은 방식으로 왼쪽 아래에 띄운다.

	void SpawnWave(const WaveEntry& wave);
	void SpawnItem(Vector pos, ItemKind kind, int32 powerValue = 1, bool burst = false, bool autoCollect = false);

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
	void clearWaveActors();	// 디버그 스테이지 점프(KEY_1/2/3) 시, 이전 스테이지에 남아있던 일반 적/적 탄환 정리
	void showStageBanner(int32 stageNum);	// 스테이지 진입 시 화면 중앙에 2초간 "스테이지 N" 배너를 띄운다.

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
	ObjectPool<LaserSegment> _laserSegmentPool;

	bool _isPaused = false;

	// 일시정지/컨티뉴 등 팝업 문구용 폰트 (타이틀 화면과 동일한 Griun Fromsol).
	HFONT _uiFont = nullptr;

	// 일시정지 메뉴: Resume/Quit Game 선택 -> Quit 선택 시 Really? Yes/No 재확인
	enum class PauseState
	{
		Menu,
		QuitConfirm
	};
	PauseState _pauseState = PauseState::Menu;
	int32 _pauseMenuSelect = 0;			// 0 = Resume, 1 = Quit Game
	bool _quitConfirmSelectYes = false;	// 기본값 No (실수로 종료 방지)

	// 스테이지 진입 시 화면 중앙에 2초간 띄우는 배너("스테이지 N").
	wstring _stageBannerText;
	float _stageBannerTimer = 0.f;

	// 폭탄 사용/보스 진입 시 화면 왼쪽 아래에 1.5초간 띄우는 일러스트.
	float _bombFaceTimer = 0.f;
	wstring _faceTextureKey = L"BombFace";

	// 엑스트라 스테이지: 요정 웨이브 없이 FullPower 3개를 깔아두고, 전부 회수되면 보스로 전환한다.
	bool _extraItemsSpawned = false;

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
	Vector _playerDeathPos;
	class Player* _player = nullptr;
	class Boss* _boss = nullptr;
	bool _bossSpawned = false;
	bool _clearedExtra = false;	// Clear 상태 진입 시점에 ExtraBoss를 잡아서 온 건지(true) Stage3Boss인지(false)

	// 컨티뉴 시스템: 최대 3번까지, 몇 번째를 선택 중인지(Yes/No)
	int32 _continueCount = 0;
	static constexpr int32 MAX_CONTINUE = 3;
	bool _continueSelectYes = true;
	GameSceneState _stateBeforeDeath = GameSceneState::Playing;	// Continue Yes 선택 시 되돌아갈 상태

	// 스테이지 클리어 결과 화면(StageResult)에서 Z를 누르면 이동할 다음 상태
	GameSceneState _stateAfterResult = GameSceneState::Playing;

	// Stage2 진입 시 텍스처를 교체하기 위해 패럴랙스 배경 두 장을 캐싱해둔다.
	class Background* _bgLayer1 = nullptr;
	class Background* _bgLayer2 = nullptr;
};


