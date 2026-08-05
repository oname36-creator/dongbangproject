#include "pch.h"
#include "Scene.h"
#include "Player.h"
#include "Enemy.h"
#include "Boss.h"
#include "Background.h"
#include "ResourceManager.h"
#include "TimeManager.h"
#include "Bullet.h"
#include "Item.h"
#include "CollisionManager.h"
#include "Effect.h"
#include "DataManager.h"
#include "ResourceData.h"
#include "WorldBG.h"
#include "GameScene.h"
#include "InputManager.h"
#include "ResultScene.h"
#include "EndingScene.h"
#include "TitleScene.h"
#include "SceneManager.h"


#include <random>
#include <format>

using namespace std;
random_device rd;
mt19937 gen(rd());

// Stage 1 웨이브 테이블. 시간 순으로 오름차순 정렬되어 있어야 한다
// (GameScene::Update()의 Playing 케이스가 _nextWaveIndex를 순서대로만 훑기 때문).
struct GameScene::WaveEntry
{
	float time;         // 스테이지 시작 후 몇 초에 등장하는지
	wstring enemyKey;   // ResourceData의 텍스처 키
	Vector pos;         // 첫 적이 등장할 위치
	int32 count;        // 가로로 나열할 개수
};

static const GameScene::WaveEntry g_waveTable[] =
{
	{ 2.0f,  L"Enemy1", {50, 100}, 4 },
	{ 6.0f,  L"Enemy2", {50, 100}, 4 },
	{ 10.0f, L"Enemy3", {50, 100}, 4 },
	{ 14.0f, L"Enemy4", {50, 100}, 4 },
};

// Stage 2 웨이브 테이블. Stage2 상태 진입 시 _stageElapsedTime을 0으로 리셋하므로
// time 값은 g_waveTable과 마찬가지로 "Stage2 시작 후 경과 시간" 기준이다.
static const GameScene::WaveEntry g_stage2WaveTable[] =
{
	{ 2.0f,  L"Enemy2", {50, 100}, 4 },
	{ 5.0f,  L"Enemy1", {50, 100}, 4 },
	{ 8.0f,  L"Enemy4", {50, 100}, 4 },
	{ 11.0f, L"Enemy3", {50, 100}, 4 },
};

// Stage 3 웨이브 테이블. 원작 동방 스테이지 구조(잡몹 웨이브 -> 중간보스 -> 잡몹 웨이브 -> 스테이지 보스)를
// 따라가기 위해 전반부/후반부로 나눈다. 각 테이블의 time은 "그 절반이 시작된 후 경과 시간" 기준
// (Stage1->Stage2 전환 시 _stageElapsedTime을 리셋하는 것과 같은 이유).
static const GameScene::WaveEntry g_stage3Wave1Table[] =
{
	{ 2.0f,  L"Enemy1", {50, 100}, 5 },
	{ 5.0f,  L"Enemy3", {50, 100}, 4 },
	{ 8.0f,  L"Enemy2", {50, 100}, 5 },
};

static const GameScene::WaveEntry g_stage3Wave2Table[] =
{
	{ 2.0f,  L"Enemy4", {50, 100}, 4 },
	{ 5.0f,  L"Enemy1", {50, 100}, 5 },
	{ 8.0f,  L"Enemy3", {50, 100}, 5 },
};

// Stage1 보스 페이즈: 기존 Fan -> Circle -> Spiral 순서 그대로.
// 각 페이즈의 timeline이 스텝 1개뿐이고 cycleLength가 옛 발사 간격과 같아서,
// "한 패턴을 고정 간격으로 무한 반복"하던 예전 동작과 똑같이 움직인다.
// (Spiral 스텝은 실제로는 연속 타이머가 쏘므로 cycleLength 값 자체는 의미 없음)
static const vector<BossPhase> g_stage1BossPhases =
{
	{ { { 0.f, BossPatternType::Fan } }, 1.0f, 50 },
	{ { { 0.f, BossPatternType::Circle } }, 1.0f, 20 },
	{ { { 0.f, BossPatternType::Spiral } }, 1.0f, 0 },
};

// Stage2 보스 페이즈: 조준 라인탄 -> 예고 판정(낙뢰) -> 원형탄.
static const vector<BossPhase> g_stage2BossPhases =
{
	{ { { 0.f, BossPatternType::AimedBurst } }, 1.0f, 100 },
	{ { { 0.f, BossPatternType::Telegraph } }, 2.5f, 50 },
	{ { { 0.f, BossPatternType::Circle } }, 1.0f, 0 },
};

// Stage3 중간보스 페이즈: 최종보스보다 약하게 페이즈 2개만.
static const vector<BossPhase> g_stage3MidBossPhases =
{
	{ { { 0.f, BossPatternType::AimedBurst } }, 1.0f, 60 },
	{ { { 0.f, BossPatternType::Fan } }, 1.0f, 0 },
};

// Stage3 최종보스 페이즈: 4페이즈 (기획서 3장 "최종 보스 4페이즈").
static const vector<BossPhase> g_stage3BossPhases =
{
	{ { { 0.f, BossPatternType::Fan } }, 1.0f, 150 },
	{ { { 0.f, BossPatternType::AimedBurst } }, 1.0f, 100 },
	{ { { 0.f, BossPatternType::Telegraph } }, 2.5f, 50 },
	{ { { 0.f, BossPatternType::Spiral }, { 0.f, BossPatternType::Cross } }, 1.0f, 0 },
};

// 생성자/소멸자를 cpp 작성하면, Scene의 인스턴스화는 cpp에서 일어남.
// ObjectPool<T> (vector<T>) 값 자체를 가지고 있는 풀을 생성하는것도,
// cpp에서 인스턴스화할때 생성됨.
// 이때는 Bullet/Enemy #include 완료 상태
GameScene::GameScene() 
{
}
GameScene::~GameScene()
{
}

void GameScene::Init()
{
	_cameraPos.x = (GWinSizeX / 2);
	_cameraPos.y = (GWinSizeY / 2);
	// Scene -> LobbyScene, GameScene, EditScene
	// Scene에 필요한 리소스 로드
	loadResources();

	// 객체생성전에 미리 풀을 생성해둔다. (2주차 원형/나선탄 대비 1000개로 상향)
	_bulletPool.Init(1000);
	_enemyPool.Init(1000);
	_itemPool.Init(100);

	// Scene에 필요한 객체 생성
	createObjects();

	TimeManager::GetInstance().AddTimer([this](){ _state = GameSceneState::Playing; }, 2.0f, false);
	// Grid 미리 생성
	_gridCountX = (int32)_mapSize.x / _gridSize;
	_gridCountY = (int32)_mapSize.y / _gridSize;

	int32 totalGridCount =_gridCountX * _gridCountY;
	_grid.resize(totalGridCount);

}

void GameScene::Cleanup()
{
	// 씬에 등장하는 모든 객체들의 delete 담당
	for (auto iter : _actors)
	{
		// CollisionManager는 싱글톤이라 씬이 바뀌어도 살아남는다. 여기서 등록 해제를
		// 안 하면, 곧 delete되거나(풀 소멸 시) 메모리가 풀릴 액터 포인터가
		// _collisionCheckList/_prev/_curr에 죽은 채로 남아 다음 GameScene에서 크래시난다.
		iter->Destroy();
		removeActor(iter);

		// Scene이 new 한 객체는 delete 해도 된다.
		if (iter->GetPool() == nullptr)
		{
			delete iter;
		}
	}
	_actors.clear();

	TimeManager::GetInstance().Clear();
}

void GameScene::Update(float deltaTime)
{
	if(InputManager::GetInstance().GetButtonDown(KeyType::PAUSE))
{
    _isPaused = !_isPaused;
    if (!_isPaused)
        _pauseState = PauseState::Menu;   // 다음에 다시 열 때는 항상 메인 메뉴부터
}

if(_isPaused)
{
    if (_pauseState == PauseState::Menu)
    {
        if (InputManager::GetInstance().GetButtonDown(KeyType::Up) ||
            InputManager::GetInstance().GetButtonDown(KeyType::Down))
        {
            _pauseMenuSelect = 1 - _pauseMenuSelect;   // 0<->1 토글
        }
        if (InputManager::GetInstance().GetButtonDown(KeyType::ATTACK))
        {
            if (_pauseMenuSelect == 0)   // Resume
            {
                _isPaused = false;
                _pauseState = PauseState::Menu;
            }
            else   // Quit Game
            {
                _pauseState = PauseState::QuitConfirm;
                _quitConfirmSelectYes = false;
            }
        }
    }
    else if (_pauseState == PauseState::QuitConfirm)
    {
        if (InputManager::GetInstance().GetButtonDown(KeyType::Up) ||
            InputManager::GetInstance().GetButtonDown(KeyType::Down))
        {
            _quitConfirmSelectYes = !_quitConfirmSelectYes;
        }
        if (InputManager::GetInstance().GetButtonDown(KeyType::ATTACK))
        {
            if (_quitConfirmSelectYes)
            {
                SceneManager::GetInstance().ChangeScene(new TitleScene());
            }
            else
            {
                _pauseState = PauseState::Menu;   // 다시 일시정지 메뉴로
            }
        }
    }
    return;
}

	if (_state != GameSceneState::Continue)
	{
		for (auto actor : _actors)
		{
			actor->Update(deltaTime);
		}
	}

	if (InputManager::GetInstance().GetButtonDown(KeyType::KEY_1))
	{
		if (_boss) _boss->Destroy();
		_bossSpawned = false;
		_boss = nullptr;
		_state = GameSceneState::Playing;
		_stageElapsedTime = 0.f;
		_nextWaveIndex = 0;
		if (_bgLayer1) _bgLayer1->ChangeTexture(L"World_BG");
		if (_bgLayer2) _bgLayer2->ChangeTexture(L"World_BG");
	}
	if (InputManager::GetInstance().GetButtonDown(KeyType::KEY_2))
	{
		if (_boss) _boss->Destroy();
		_bossSpawned = false;
		_boss = nullptr;
		_state = GameSceneState::Stage2;
		_stageElapsedTime = 0.f;
		_nextStage2WaveIndex = 0;
		if (_bgLayer1) _bgLayer1->ChangeTexture(L"Stage2BG");
		if (_bgLayer2) _bgLayer2->ChangeTexture(L"Stage2BG");
	}
	if (InputManager::GetInstance().GetButtonDown(KeyType::KEY_3))
	{
		if (_boss) _boss->Destroy();
		_bossSpawned = false;
		_boss = nullptr;
		_state = GameSceneState::Stage3;
		_stageElapsedTime = 0.f;
		_nextStage3WaveIndex = 0;
		_stage3Wave2 = false;
		if (_bgLayer1) _bgLayer1->ChangeTexture(L"Stage2BG");
		if (_bgLayer2) _bgLayer2->ChangeTexture(L"Stage2BG");
	}


	switch(_state)
	{
		case GameSceneState :: Ready : 
			break;
		case GameSceneState :: Playing : 
		
			_stageElapsedTime += deltaTime;
			while (_nextWaveIndex < std::size(g_waveTable) && g_waveTable[_nextWaveIndex].time <= _stageElapsedTime)
			{
				SpawnWave(g_waveTable[_nextWaveIndex]);
				_nextWaveIndex++;
			}
			if(_player == nullptr)
			{
				onPlayerDead(); // _player->GetHp() = 0 이미 player가 nullptr 이기 때문에 위험하다.
			}
			else if (_nextWaveIndex >= (int32)std::size(g_waveTable))
			{
				// 모든 웨이브를 소진했으니 보스 스테이지로 전환
				_state = GameSceneState :: Boss;
			}
			break;
		case GameSceneState :: Boss :
			// Boss 상태에 처음 들어온 프레임에만 스폰 (한 번만 생성)
			if (!_bossSpawned)
			{
					Boss* boss = new Boss();
					boss->Init(Vector(GWinSizeX * 0.5f, -50.f), L"Boss", g_stage1BossPhases, 100);
					_reservedAdd.push_back(boss);
					_boss = boss;
					_bossSpawned = true;
			}
			else if (_player == nullptr)
			{
				onPlayerDead();
			}
			else if(_boss == nullptr)
			{
				// Stage1의 _stageElapsedTime을 그대로 물려받으면 g_stage2WaveTable의
				// time 값과 어긋나므로 Stage2 시작 시점 기준으로 리셋한다.
				_stageElapsedTime = 0.f;
				if (_bgLayer1) _bgLayer1->ChangeTexture(L"Stage2BG");
				if (_bgLayer2) _bgLayer2->ChangeTexture(L"Stage2BG");
				_state = GameSceneState::Stage2;
			}

			break;
		case GameSceneState :: Stage2 :
			_stageElapsedTime += deltaTime;
			while (_nextStage2WaveIndex < std::size(g_stage2WaveTable) && g_stage2WaveTable[_nextStage2WaveIndex].time <= _stageElapsedTime)
			{
				SpawnWave(g_stage2WaveTable[_nextStage2WaveIndex]);
				_nextStage2WaveIndex++;
			}
			if(_player == nullptr)
			{
				onPlayerDead(); // _player->GetHp() = 0 이미 player가 nullptr 이기 때문에 위험하다.
			}
			else if (_nextStage2WaveIndex >= (int32)std::size(g_stage2WaveTable))
			{
				// 모든 웨이브를 소진했으니 Stage2 보스로 전환
				_bossSpawned = false;
				_state = GameSceneState :: Stage2Boss;
			}
			break;
		case GameSceneState :: Stage2Boss :
			// Boss 상태와 동일한 패턴: 처음 들어온 프레임에만 스폰, 죽으면 Clear로 전환
			if (!_bossSpawned)
			{
				Boss* boss = new Boss();
				boss->Init(Vector(GWinSizeX * 0.5f, -50.f), L"Boss", g_stage2BossPhases, 150);
				_reservedAdd.push_back(boss);
				_boss = boss;
				_bossSpawned = true;
			}
			else if (_player == nullptr)
			{
				onPlayerDead();
			}
			else if (_boss == nullptr)
			{
				// Stage3도 Stage1->Stage2 전환과 마찬가지로 경과시간/웨이브 인덱스를 리셋한다.
				_stageElapsedTime = 0.f;
				_nextStage3WaveIndex = 0;
				_stage3Wave2 = false;
				_state = GameSceneState :: Stage3;
			}
			break;
		case GameSceneState :: Stage3 :
			_stageElapsedTime += deltaTime;
			if (!_stage3Wave2)
			{
				// 전반부 웨이브 -> 다 쓰면 중간보스로.
				while (_nextStage3WaveIndex < std::size(g_stage3Wave1Table) && g_stage3Wave1Table[_nextStage3WaveIndex].time <= _stageElapsedTime)
				{
					SpawnWave(g_stage3Wave1Table[_nextStage3WaveIndex]);
					_nextStage3WaveIndex++;
				}
				if (_player == nullptr)
				{
					onPlayerDead();
				}
				else if (_nextStage3WaveIndex >= (int32)std::size(g_stage3Wave1Table))
				{
					_bossSpawned = false;
					_state = GameSceneState :: Stage3MidBoss;
				}
			}
			else
			{
				// 후반부 웨이브 -> 다 쓰면 최종보스로.
				while (_nextStage3WaveIndex < std::size(g_stage3Wave2Table) && g_stage3Wave2Table[_nextStage3WaveIndex].time <= _stageElapsedTime)
				{
					SpawnWave(g_stage3Wave2Table[_nextStage3WaveIndex]);
					_nextStage3WaveIndex++;
				}
				if (_player == nullptr)
				{
					onPlayerDead();
				}
				else if (_nextStage3WaveIndex >= (int32)std::size(g_stage3Wave2Table))
				{
					_bossSpawned = false;
					_state = GameSceneState :: Stage3Boss;
				}
			}
			break;
		case GameSceneState :: Stage3MidBoss :
			if (!_bossSpawned)
			{
				Boss* boss = new Boss();
				boss->Init(Vector(GWinSizeX * 0.5f, -50.f), L"Boss", g_stage3MidBossPhases, 130);
				_reservedAdd.push_back(boss);
				_boss = boss;
				_bossSpawned = true;
			}
			else if (_player == nullptr)
			{
				onPlayerDead();
			}
			else if (_boss == nullptr)
			{
				// 중간보스 격파 -> 후반부 웨이브 재생을 위해 경과시간/인덱스를 리셋한다.
				_stageElapsedTime = 0.f;
				_nextStage3WaveIndex = 0;
				_stage3Wave2 = true;
				_state = GameSceneState :: Stage3;
			}
			break;
		case GameSceneState :: Stage3Boss :
			if (!_bossSpawned)
			{
				Boss* boss = new Boss();
				boss->Init(Vector(GWinSizeX * 0.5f, -50.f), L"Boss", g_stage3BossPhases, 250);
				_reservedAdd.push_back(boss);
				_boss = boss;
				_bossSpawned = true;
			}
			else if (_player == nullptr)
			{
				onPlayerDead();
			}
			else if (_boss == nullptr)
			{
				_stageElapsedTime = 0.f;
				_state = GameSceneState :: Clear;
			}
			break;
		case GameSceneState :: Clear :
			SceneManager::GetInstance().ChangeScene(new EndingScene(_score));
			break;
		case GameSceneState :: GameOver : 
			if(InputManager::GetInstance().GetButtonDown(KeyType::ATTACK))
			{
				SceneManager::GetInstance().ChangeScene(new ResultScene(_score));
			}
			break;
		case GameSceneState::Continue:
			if (InputManager::GetInstance().GetButtonDown(KeyType::Up) ||
				InputManager::GetInstance().GetButtonDown(KeyType::Down))
			{
				_continueSelectYes = !_continueSelectYes;
			}
			if (InputManager::GetInstance().GetButtonDown(KeyType::ATTACK))
			{
				if (_continueSelectYes)
				{
					_continueCount++;
					
					// createObjects()의 723~729번 줄과 같은 패턴으로 플레이어 재생성
					Player* player = new Player();
					
					player->Init();
					player->SetPos(Vector(GWinSizeX * 0.5f, 400));
					player->SetInvincible(3.3f);
					_reservedAdd.push_back(player);
					_player = player;

					_state = _stateBeforeDeath;   // 죽기 전 스테이지 상태로 복귀
				}
				else
				{
					_state = GameSceneState::GameOver;
				}
			}
			break;

	}

	// 삭제가 필요한 애들은 삭제
	// 1번 방식으로 '삭제 여부' 걸러도 되고,
	// 2번 방식으로 '삭제 여부' 걸러도 되고
	std::erase_if(_actors, [this](Actor* actor)
		{
			// 1번 방식은, _actor에서 제거는 되는데,
			// 제거되기전에 아래 delete + removeActor 함수 호출해야해서
			// 2번 방식으로 별도 리스트를 관리하는게 좋겟다.
			//return actor->GetPendingKill(); 
			return _reservedRemove.contains(actor);
		});

	// 실제 메모리 해제 까먹었따.
	for (auto deleteActor : _reservedRemove)
	{
		// 삭제되는 Actor
		removeActor(deleteActor);

		// 해당 Actor가 풀에서 태어난 경우에는 반환
		if (deleteActor->GetPool())
		{
			// 해당 풀에다가 반환
			deleteActor->GetPool()->Return(deleteActor);
		}
		else
		{
			// new 태어난 경우는 delete
			delete deleteActor;
		}
	}

	// 추가가 필요한 애들은 추가
	// 1번 방식 : 매번 push_back 할때마다 비용 지불
	//for (auto iter : _reservedAdd)
	//{
	//	// vector
	//	// capacity, size
	//	// 새로 원소를 집어넣을떄 capacity 부족시, 메모리 추가 할당
	//	_actors.push_back(iter);
	//}

	// 2번 방식 :
	// 여긴, reserverdAdd 에 10개가 있을경우
	// vector를 한번에 10개 늘리고 복사해와서, 재할당이 1번 일어난다.
	//_actors.insert(_actors.end(), _reservedAdd.begin(), _reservedAdd.end());

	//-> 미리 한번만 할당해놓고, 복사하기
	_actors.reserve(_actors.size() + _reservedAdd.size()); // 개수X, Capacity(메모리)
	for (Actor* actor : _reservedAdd)
	{
		// 추가되는 Actor
		registerActor(actor);		
		_actors.push_back(actor);	// reserve() 함수로 미리 capacity 확보해뒀다.
	}

	// 지연리스트 초기화
	_reservedAdd.clear();
	_reservedRemove.clear();

	// 그리드 갱신 : 초기화 -> 재갱신 이방식이 마음에 안든다면,
	// Actor의 위치가 변경될때마다 Grid 의 위치를 갱신해주는 방식을 하면 된다.
	// 즉, 아래 코드는 다 사라지고 Actor가 Scene에게 요청을 해서, Grid 갱신한다.
	{
		// 모든 Actor의 최신화된 좌표 기준으로 Grid 갱신
		// 이전프레임에 있었던 Grid 정보는 초기화
		for (GridInfo& grid : _grid)
		{
			grid.actors.clear();
		}

		// 게임 특성상 매프레임 위치 변경이 있으니깐, 
		// 그냥 전체 순회하면서 Grid 등록을 해준다.
		// 전체 순회니깐, 어차피 또 성능적인 측면의 이점이 없는거 아닌가요.
		// O(N*M) -> O(N)
		for (auto actor : _actors)
		{
			updateGrid(actor);
		}
	}


	// 카메라는 Init()에서 화면 중앙으로 고정해뒀다 (종스크롤 STG는 화면 고정 + 배경 스크롤).
}

void GameScene::Render(HDC hdc)
{
	for (auto list : _renderList)
	{
		for (auto actor : list)
		{
			actor->Render(hdc);
		}
	}

	if (_isPaused)
{
    if (_pauseState == PauseState::Menu)
    {
        wstring msg = L"PAUSED";
        wstring resumeText = std::format(L"{0} Resume", _pauseMenuSelect == 0 ? L">" : L" ");
        wstring quitText = std::format(L"{0} Quit Game", _pauseMenuSelect == 1 ? L">" : L" ");

        ::TextOut(hdc, GWinSizeX/2 - 40, GWinSizeY/2 - 30, msg.c_str(), (int32)msg.size());
        ::TextOut(hdc, GWinSizeX/2 - 40, GWinSizeY/2, resumeText.c_str(), (int32)resumeText.size());
        ::TextOut(hdc, GWinSizeX/2 - 40, GWinSizeY/2 + 20, quitText.c_str(), (int32)quitText.size());
    }
    else if (_pauseState == PauseState::QuitConfirm)
    {
        wstring msg = L"Really?";
        wstring yesText = std::format(L"{0} Yes", _quitConfirmSelectYes ? L">" : L" ");
        wstring noText = std::format(L"{0} No", !_quitConfirmSelectYes ? L">" : L" ");

        ::TextOut(hdc, GWinSizeX/2 - 30, GWinSizeY/2 - 30, msg.c_str(), (int32)msg.size());
        ::TextOut(hdc, GWinSizeX/2 - 30, GWinSizeY/2, yesText.c_str(), (int32)yesText.size());
        ::TextOut(hdc, GWinSizeX/2 - 30, GWinSizeY/2 + 20, noText.c_str(), (int32)noText.size());
    }
}


	if (_state == GameSceneState::Continue)
	{
		wstring msg = L"Continue?";
		wstring yesText = std::format(L"{0} Yes", _continueSelectYes ? L">" : L" ");
		wstring noText = std::format(L"{0} No", !_continueSelectYes ? L">" : L" ");

		::TextOut(hdc, GWinSizeX / 2 - 30, GWinSizeY / 2 - 20, msg.c_str(), (int32)msg.size());
		::TextOut(hdc, GWinSizeX / 2 - 30, GWinSizeY / 2, yesText.c_str(), (int32)yesText.size());
		::TextOut(hdc, GWinSizeX / 2 - 30, GWinSizeY / 2 + 20, noText.c_str(), (int32)noText.size());
	}
}

void GameScene::DeleteActor(Actor* actor)
{
	// 여기에서 즉시 삭제하지 않는다.
	// vector 빼고, delete 해주고.
	//	_actors.erase

	// 지연 삭제
	// 추가적인 리스트에 넣고, 나중에 한번에 삭제
	_reservedRemove.insert(actor);
}

void GameScene::SpawnItem(Vector pos, ItemKind kind, int32 powerValue, bool burst)
{
	Item* item = _itemPool.Acquire();
	if ( item == nullptr)
		return;

	item->Init(pos, kind, powerValue, burst);
	_reservedAdd.push_back(item);
}

void GameScene::CreateBullet(Vector pos, BulletType type, Vector dir, float speed, bool isHoming, float turnSpeed)
{
	
	Bullet* bullet = _bulletPool.Acquire(); // new Bullet();


	if(bullet == nullptr)
		return;

	bullet->Init(type, dir, speed, isHoming, turnSpeed);
	bullet->SetPos(pos);

	_reservedAdd.push_back(bullet);
}

void GameScene::FireStraight(Vector pos, BulletType type, Vector dir, float speed)
{
	CreateBullet(pos, type, dir, speed);
}

void GameScene::FireAimed(Vector pos, BulletType type,Vector targetPos, float speed)
{
	Vector dir = targetPos - pos;
	dir.Normalize();
	CreateBullet(pos, type, dir, speed);
}

void GameScene::FireCircle(Vector pos, BulletType type, int32 count, float speed)
{
	
	for(int32 i = 0; i < count; ++i)
	{
		float radian = DegreeToRadian(i*(360.f/count));
		CreateBullet(pos, type, Vector(cosf(radian), sinf(radian)), speed);
	}
}

void GameScene::FireSpiral(Vector pos, BulletType type, int32 count, float speed, float& rotationAngle, float rotationSpeed)
{
	for(int32 i = 0; i < count; ++i)
		{
			float radian = DegreeToRadian(rotationAngle + (i*(360.f/count)));
			CreateBullet(pos, type, Vector(cosf(radian), sinf(radian)), speed);
		}

	rotationAngle += rotationSpeed; // 다음 호출 때 이어질 수 있도록 각도 누적
}

void GameScene::FireFan(Vector pos, BulletType type, Vector dir, float anglespread, int32 count, float speed)
{
	float baseAngle = atan2f(dir.y, dir.x); // dir 방향의 각도 (라디안)

	for(int32 i = 0; i < count; ++i)
	{
		float offsetDeg = i * (anglespread / count) - anglespread / 2.f; // 중심 기준 좌우 대칭 오프셋
		float radian = baseAngle + DegreeToRadian(offsetDeg);
		CreateBullet(pos, type, Vector(cosf(radian), sinf(radian)), speed);
	}
}

void GameScene::FireRandom(Vector pos, BulletType type, int32 count, float speed)
{
	uniform_real_distribution<float> randir(0, 360);
	for(int32 i = 0; i < count; ++i)
	{
		float random_dir = randir(gen);
		float radian = DegreeToRadian(random_dir);
		CreateBullet(pos, type, Vector(cosf(radian), sinf(radian)), speed);
	}
}

void GameScene::FireHoming(Vector pos, BulletType type, Vector dir, float speed, float turnSpeed)
{
	CreateBullet(pos, type, dir, speed, true, turnSpeed);
}

Actor* GameScene::FindNearestEnemy(Vector pos)
{
	Actor* nearest = nullptr;
	float nearestDistSq = 0.f;

	for (Actor* enemy : GetRenderList(RenderLayer::Enemy))
	{
		Vector diff = enemy->GetPos() - pos;
		float distSq = diff.LengthSquared();
		if (nearest == nullptr || distSq < nearestDistSq)
		{
			nearest = enemy;
			nearestDistSq = distSq;
		}
	}

	if (_boss != nullptr)
	{
		Vector diff = _boss->GetPos() - pos;
		float distSq = diff.LengthSquared();
		if (nearest == nullptr || distSq < nearestDistSq)
		{
			nearest = _boss;
			nearestDistSq = distSq;
		}
	}

	return nearest;
}
void GameScene::FireGrid(Vector origin, BulletType type, Vector dir, int32 raws, int32 cols, float spacingX, float spacingY, float speed)
{
	dir.Normalize();

	for(int32 r = 0; r < raws; ++r)
	{
		for(int32 c = 0; c < cols; ++c)
		{
			Vector spawnPos = Vector(
				origin.x + (c * spacingX),
				origin.y + (r * spacingY)
			);
			CreateBullet(spawnPos, type, dir, speed);
		}
	}
	
}

void GameScene::FireCross(float y, BulletType type, float speed)
{
		Vector leftPos(0.f, y);
		Vector rightPos((float)GWinSizeX,y);

		Vector leftDir(1.f, 1.f);
		leftDir.Normalize();
		Vector rightDir(-1.f,1.f);
		rightDir.Normalize();

		CreateBullet(leftPos, type, leftDir, speed);
		CreateBullet(rightPos, type, rightDir, speed);
}

void GameScene::CreateEffect(Vector pos)
{
	Effect* effect = new Effect();
	effect->Init(L"Effect");
	effect->SetPos(pos);

	_reservedAdd.push_back(effect);
}

void GameScene::ClearEnemyBullets()
{
	const vector<Actor*>& bullets = GetRenderList(RenderLayer::Bullet);
	for (Actor* actor : bullets)
	{
		if (actor->GetActorType() == ActorType::EnemyBullet)
		{
			actor->Destroy();
		}
	}
}

Vector GameScene::ConvertWorldToScreen(Vector worldPos)
{
	Vector offset;
	offset.x = _cameraPos.x - (GWinSizeX / 2);
	offset.y = _cameraPos.y - (GWinSizeY / 2);

	Vector screenPos = worldPos - offset;
	return screenPos;
}

const vector<Actor*>& GameScene::GetRenderList(RenderLayer layer) const
{
	if ((int32)layer < 0 || layer >= RenderLayer::Count)
	{
		static vector<Actor*> emptyList;
		return emptyList;
	}
	return _renderList[(int32)layer];
}

void GameScene::loadResources()
{
	// DataManager에 가서 GameScene에 필요한 모든 데이터를 다 로드해달라고 요청
	ResourceData* data = DataManager::GetInstance().FindData<ResourceData>(L"ResourceData");
	if (data)
	{
		// 새로운 텍스처 로드가 필요하면, json에 추가하면 끝!
		for (auto iter : data->_gameSceneData)
		{
			const ResourceData::Item& item = iter.second;
			ResourceManager::GetInstance().LoadTexture(
				iter.first, 
				item.fileName, 
				item.transparent,
				item.countY,
				item.countX,
				item.dur);
		}
	}

}

void GameScene::createObjects()
{
	// 윈도우 크기와 1:1맞는 고정 배경
	if(true)
	{
		Background* bg = new Background();
		bg->Init(L"World_BG",200.0f);
		_reservedAdd.push_back(bg);
		_bgLayer1 = bg;
	}
	if(true)
	{
		Background* bg = new Background();
		bg->Init(L"World_BG",50.0f);
		_reservedAdd.push_back(bg);
		_bgLayer2 = bg;
	}

	// 윈도우 크기보다 훨씬큰 배경
	if(true)
	{
		WorldBG* bg = new WorldBG();
		bg->Init();

		_mapSize = Vector(GWinSizeX, GWinSizeY);
	}

	// 플레이어
	Player* player = new Player();
	player->Init();
	player->SetPos(Vector(GWinSizeX * 0.5f, 400));
	_reservedAdd.push_back(player);

	// 플레이어 객체를 캐싱해두자.
	_player = player;

	// 적 스폰은 이제 웨이브 테이블(g_waveTable)이 GameScene::Update()의 Playing
	// 케이스에서 스테이지 경과 시간을 보고 SpawnWave()를 호출하는 방식으로 진행된다.
}

// Scene에 등록되는 Actor들이 모두 해야할일
void GameScene::registerActor(Actor* actor)
{
	if (nullptr == actor)
		return;

	if (actor->GetRenderLayer() >= RenderLayer::Count)
		return;

	// 렌더링 순서에 맞게 리스트 갱신
	_renderList[(int32)actor->GetRenderLayer()].push_back(actor);

	// 충돌체크가 필요하다면, 충돌체크 등록
	if (actor->GetCollider())
	{
		CollisionManager::GetInstance().AddActor(actor);
	}
}

void GameScene::onPlayerDead()
{
	if(_continueCount < MAX_CONTINUE)
	{
		_stateBeforeDeath = _state;
		_continueSelectYes = true;
		_state = GameSceneState::Continue;
	}
	else
	{
		_state = GameSceneState::GameOver;
	}
}

// Scene에 제거되는 Actor들이 모두 해야할일
void GameScene::removeActor(Actor* actor)
{
	if (nullptr == actor)
		return;

	// 삭제해야하는 객체가 Player였다면, 댕글링포인터를 예방하기 위해
	// 캐싱해두고 있던 포인터도 갱신해주자.
	if (actor == _player)
	{
		_player = nullptr;
	}

	if (actor == _boss)
	{
		_boss = nullptr;
	}

	if (actor->GetRenderLayer() >= RenderLayer::Count)
		return;

	// RenderList vector에서 찾아서 제거
	std::erase_if(_renderList[(int32)actor->GetRenderLayer()],
		[actor](Actor* iter)
		{
			return iter == actor;
		});

	// 삭제될 Actor포인터를 CollisionManager 에서 Enter/Exit 비교를 위해
	// 포인터를 저장하고 있기 때문에, 충돌체크에서도 빼자.
	if (actor->GetCollider())
	{
		CollisionManager::GetInstance().RemoveActor(actor);
	}
}




void GameScene::SpawnWave(const WaveEntry& wave)
{
	int32 xDelta = GWinSizeX / wave.count;

	for(int32 i = 0; i < wave.count; ++i)
	{
		Enemy* enemy = _enemyPool.Acquire();
		if(nullptr == enemy)
		return;

		enemy->Init(Vector{wave.pos.x + (xDelta * i), wave.pos.y}, wave.enemyKey);
		_reservedAdd.push_back(enemy);
	}
}

void GameScene::updateGrid(Actor* actor)
{
	Cell cell = Cell::ConvertToCell(actor->GetPos(), _gridSize);

	// Cell의 범위를 체크해서, 굳이 관리가 필요없는 좌표의 경우는 무시
	if (cell.iX < 0 || cell.iY < 0 || cell.iX >= _gridCountX || cell.iY >= _gridCountY)
		return;

	// 1차원 배열의 인덱스로 변환을 해줘야한다.
	int32 index = cell.iY * _gridCountX + cell.iX; // Sprite Anim 방식과 비슷

	// 배열을 [] 인덱스 기법으로 접근할때는 범위체크 해주자.
	if (index >= 0 && index < _grid.size())
	{
		_grid[index].actors.push_back(actor);
	}
}

const GridInfo& GameScene::GetGridInfo(const Cell& cell)
{
	// per-axis bounds check to prevent index wraparound
	static GridInfo emptyGridInfo{};
	if (cell.iX < 0 || cell.iX >= _gridCountX || cell.iY < 0 || cell.iY >= _gridCountY)
		return emptyGridInfo;

	int32 index = cell.iY * _gridCountX + cell.iX;
	if (index >= 0 && index < _grid.size())
	{
		GridInfo& gridInfo = _grid[index];
		return gridInfo;
	}

	// null object 패턴.
	return emptyGridInfo;
}