#include "pch.h"
#include "Game.h"
#include "Scene.h"
#include "Player.h"
#include "Enemy.h"
#include "Boss.h"
#include "BossIllusion.h"
#include "Background.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "TimeManager.h"
#include "Bullet.h"
#include "Item.h"
#include "LaserSegment.h"
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
#include "AudioManager.h"
#include "SaveManager.h"
#include "Laser.h"

#include <random>
#include <format>

using namespace std;

namespace
{
	constexpr float BOMB_FACE_DURATION = 1.5f;
	constexpr float BOMB_FACE_OUTRO = 0.4f;	// 마지막 0.4초 동안 커지면서 투명해진다.
	constexpr float BOMB_FACE_SCALE = 1.3f;	// 원본 텍스처 크기 대비 확대 배율
	constexpr float BOMB_FACE_OUTRO_SCALE = 0.3f;	// 아웃트로 동안 추가로 커지는 비율
}
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
	EntryDirection entryDir = EntryDirection::Top;
	float hpMultiplier =1.f;
};

static const GameScene::WaveEntry g_waveTable[] =
{
	{ 2.0f,  L"Enemy1", {50, 130}, 8 },
	{ 9.0f,  L"Enemy2", {50, 130}, 8 },
	{ 16.0f, L"Enemy3", {50, 130}, 8 },
	{ 23.0f, L"Enemy4", {50, 130}, 8 },
};

// Stage 2 웨이브 테이블. Stage2 상태 진입 시 _stageElapsedTime을 0으로 리셋하므로
// time 값은 g_waveTable과 마찬가지로 "Stage2 시작 후 경과 시간" 기준이다.
static const GameScene::WaveEntry g_stage2WaveTable[] =
{
	{ 2.0f,  L"Enemy2", {50, 130}, 8, EntryDirection::Top, 4.f },
	{ 8.0f,  L"Enemy1", {50, 130}, 8, EntryDirection::Top, 4.f },
	{ 14.0f, L"Enemy4", {50, 130}, 8, EntryDirection::Top, 4.f },
	{ 20.0f, L"Enemy3", {50, 130}, 8, EntryDirection::Top, 4.f },
};

// Stage 3 웨이브 테이블. 원작 동방 스테이지 구조(잡몹 웨이브 -> 중간보스 -> 잡몹 웨이브 -> 스테이지 보스)를
// 따라가기 위해 전반부/후반부로 나눈다. 각 테이블의 time은 "그 절반이 시작된 후 경과 시간" 기준
// (Stage1->Stage2 전환 시 _stageElapsedTime을 리셋하는 것과 같은 이유).
static const GameScene::WaveEntry g_stage3Wave1Table[] =
{
	{ 2.0f,  L"Enemy1", {50, 130}, 8, EntryDirection::Top, 8.f },
	{ 8.0f,  L"Enemy3", {50, 130}, 8, EntryDirection::Top, 8.f },
	{ 14.0f, L"Enemy2", {50, 130}, 8, EntryDirection::Top, 8.f },
};

static const GameScene::WaveEntry g_stage3Wave2Table[] =
{
	{ 2.0f,  L"Enemy4", {50, 130}, 8, EntryDirection::Top, 8.f },
	{ 8.0f,  L"Enemy1", {50, 130}, 8, EntryDirection::Top, 8.f },
	{ 14.0f, L"Enemy3", {50, 130}, 8, EntryDirection::Top, 8.f },
};

// Stage1 보스 페이즈: 기존 Fan -> Circle -> Spiral 순서 그대로.
// 각 페이즈의 timeline이 스텝 1개뿐이고 cycleLength가 옛 발사 간격과 같아서,
// "한 패턴을 고정 간격으로 무한 반복"하던 예전 동작과 똑같이 움직인다.
// (Spiral 스텝은 실제로는 연속 타이머가 쏘므로 cycleLength 값 자체는 의미 없음)
static const vector<BossPhase> g_stage1BossPhases =
{
	{ { { 0.f, BossPatternType::Fan } }, 1.0f, 100 },
	{ { { 0.f, BossPatternType::Circle } }, 1.0f, 40 },
	{ { { 0.f, BossPatternType::Spiral } }, 1.0f, 0 },
};

// Stage2 보스 페이즈: 조준 라인탄 -> 무작위 난사 -> 원형탄.
static const vector<BossPhase> g_stage2BossPhases =
{
	{
		{
			{ 0.f, BossPatternType::AimedBurst }, 
			{ 0.f, BossPatternType::Circle } , 
			{ 1.f, BossPatternType::Circle } , 
		}, 2.0f, 800
	},
	{
		{
			{ 0.0f, BossPatternType::Random }, { 0.3f, BossPatternType::Random },
			{ 0.6f, BossPatternType::Random }, { 0.9f, BossPatternType::Random },
			{ 1.2f, BossPatternType::Random }, { 1.5f, BossPatternType::Random },
			{ 1.8f, BossPatternType::Random }, { 2.1f, BossPatternType::Random },
		}, 2.5f, 400
	},
	{
		{
			{ 0.f, BossPatternType::CircleDelayedAimed },
			{ 0.5f, BossPatternType::CircleDelayedRandom },
		}, 1.0f, 0
	},
};

// Stage3 중간보스 페이즈: 최종보스보다 약하게 페이즈 3개.
// 가운데 Spiral 페이즈는 팔 1개(_spiralArmCount=1)로 촘촘하고 빠르게 도는 전용 세팅을 쓴다 (Boss::Init 참고).
static const vector<BossPhase> g_stage3MidBossPhases =
{
	{ { { 0.f, BossPatternType::AimedBurst }, { 0.f, BossPatternType::Random } }, 1.0f, 320 },
	{ { { 0.f, BossPatternType::Spiral } }, 1.0f, 160 },
	{ { { 0.f, BossPatternType::Fan }, { 0.f, BossPatternType::RandomDelayedRandom } }, 1.0f, 0 },
};

// Stage3 최종보스 페이즈: 4페이즈 (기획서 3장 "최종 보스 4페이즈").
static const vector<BossPhase> g_stage3BossPhases =
{
	// AimedSpread는 1초간(30발) 뿌리고 남은 1.5초는 쉬도록 사이클을 2.5초로 늘림.
	// Fan 대신 Circle(30발)을 0초/1초에 찍어둠 (사이클이 2.5초라 다음 Circle까지 간격이 1초/1.5초로 번갈아진다).
	{ { { 0.f, BossPatternType::Circle }, { 0.f, BossPatternType::AimedSpread }, { 1.f, BossPatternType::Circle } }, 2.5f, 1200 },
	{ { { 0.f, BossPatternType::AimedBurst }, { 0.f, BossPatternType::Grid } }, 1.0f, 800 },
	// Telegraph(화면 상단 좌우 고정 2곳) + Circle(30발, 보스 공용 Circle 설정 재사용) + Spiral(중간보스 2페이즈와 동일 스펙: 팔1/15도/0.0133초) 동시 진행.
	{ { { 0.f, BossPatternType::Telegraph }, { 0.f, BossPatternType::Circle }, { 0.f, BossPatternType::Spiral } }, 2.5f, 400 },
	// AimedBurst(가속 연사)를 4페이즈로 이동. Spiral+Cross는 계속 도는 연속 발사라 여기 0.5초 지점에 겹쳐 넣는다.
	{ { { 0.f, BossPatternType::Spiral }, { 0.f, BossPatternType::Cross }, { 0.5f, BossPatternType::AimedBurst } }, 1.0f, 0 },
};

// 엑스트라 보스 콘텐츠 확정 전, 원작 수준 탄막 밀도에서 프레임이 버티는지 확인하기 위한
// 스트레스 테스트용 임시 단일 페이즈(Spiral 6줄기 + Circle 60발 + Random 80발이 1초 사이클로 계속 겹쳐 나간다).
static const vector<BossPhase> g_extraBossTestPhases =
{
	{ { { 0.f, BossPatternType::Spiral }, { 0.f, BossPatternType::Circle }, { 0.5f, BossPatternType::Circle },
		{ 0.f, BossPatternType::Random }, { 0.5f, BossPatternType::Random } }, 1.0f, 0 },
};

// 엑스트라 보스 1페이즈: "이중 나선 우리". Spiral(연속)을 기본 회피 루트로 삼고,
// AimedSpread/Telegraph가 중간중간 리듬을 깬다. 보스는 이 페이즈 동안 화면 중앙에 고정된다
// (Boss::Init()의 fixedPosPhaseIndex=0, GameScene.cpp의 스폰 호출 참고).
// hpThreshold는 절대 HP 값. maxHp=5000 기준 10%(500)가 깎이면(4500 이하) 2페이즈로 전환.
static const vector<BossPhase> g_extraBossPhases =
{
	{ { { 0.f, BossPatternType::Spiral }, { 1.5f, BossPatternType::AimedSpread }, { 3.0f, BossPatternType::Telegraph },
		{ 4.5f, BossPatternType::AimedSpread } }, 6.0f, 4500 },
	// 2페이즈: ConvergingBurst — 플레이어 방향으로 3발이 느리게/보통/빠르게 동시에 나갔다가 같은 속도로 수렴해서 정렬된다.
	{ { { 0.f, BossPatternType::ConvergingBurst } }, 1.0f, 4000 },
	// 3페이즈: 마법진 6개가 화면 테두리를 돌면서 조준탄을 계속 쏜다.
	{ { { 0.f, BossPatternType::BorderAimedBurst } }, 1.0f, 3500 },
	// 4페이즈
	{ { { 0.f, BossPatternType::Leavatein } }, 1.0f, 3000 },
	// 5페이즈: 분신 3체 소환("Four of a Kind"류). 본체는 1초마다 Circle/Fan 중 하나를 무작위로 쏘고,
	// 분신 3체(Boss::spawnIllusionClones)도 각자 독립된 타이머로 같은 방식(무작위 택1)을 0.2초씩 시간차를 두고 쏜다.
	// 진짜/가짜 구분은 순전히 맞혀봐야 알 수 있음 — 분신을 맞혀도 이 HP는 전혀 줄지 않는다.
	{ { { 0.f, BossPatternType::IllusionBurst } }, 1.0f, 2500 },
	// 6페이즈: "카고메 카고메". 격자탄 웨이브(가로+세로 / 대각선 번갈아 생성)와 큰 탄 리듬(단발/부채꼴)이
	// 서로 독립된 타이머로 동시에 진행되고, 큰 탄이 지나가는 자리의 격자탄은 흐트러진다(Boss::updateKagomeDisruption).
	{ { { 0.f, BossPatternType::Kagome } }, 1.0f, 2000 },
	// 7페이즈: "사랑의 미로". Spiral(연속 회전)과 Circle(주기적 링)이 동시에 돌면서, 항상 같은 20도
	// 빈 구간을 비우고 쏜다. 그 구간은 Spiral이 팔을 새로 쏠 때마다 양옆 중 무작위로 한 칸씩 이동한다.
	{ { { 0.f, BossPatternType::LoveMaze } }, 1.0f, 1500 },
	// 8페이즈: "스타보우 브레이크". 대각선("/" 또는 "\")이나 십자 대형 중 하나를 무작위로 골라 색깔별
	// 탄이 한 알씩 순차 스폰되고, 각 탄은 잠깐 떠올랐다가 가속하며 떨어진다. 생성 -> 2초 대기 반복.
	{ { { 0.f, BossPatternType::StarbowBreak } }, 1.0f, 1000 },
	// 9페이즈: "과거를 새기는 시계". 위(270도)/아래(120도, 조준) 부채꼴을 같은 타이머로 동시에 쏘고,
	// 날개 4장짜리 프로펠러 레이저 2개가 화면 양쪽에서 서로 반대 방향으로 오간다.
	{ { { 0.f, BossPatternType::PastClock } }, 1.0f, 500 },
	// 10페이즈: "Q.E.D. 495년의 파문". 원형탄(40발)이 최초엔 보스 위치에서, 이후엔 화면 상단 무작위
	// 위치에서 계속 터진다. 좌/우/위 벽에서 딱 1번 반사하고, HP 100 깎일 때마다 계단식으로 빨라진다.
	{ { { 0.f, BossPatternType::QED } }, 1.0f, 0 }
};

// 생성자/소멸자를 cpp 작성하면, Scene의 인스턴스화는 cpp에서 일어남.
// ObjectPool<T> (vector<T>) 값 자체를 가지고 있는 풀을 생성하는것도,
// cpp에서 인스턴스화할때 생성됨.
// 이때는 Bullet/Enemy #include 완료 상태
GameScene::GameScene(bool startInExtra)
	: _startInExtra(startInExtra)
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

	// 좌상단 좌표로 바로 그릴 수 있게(센터링 보정 안 받게) 표시 위치를 직접 계산한다.
	const wchar_t* faceTextureKeys[] = { L"BombFace", L"Boss1Face", L"Boss2Face", L"Boss3MidFace", L"Boss3Face" };
	for (const wchar_t* key : faceTextureKeys)
	{
		Texture* faceTex = ResourceManager::GetInstance().GetTexture(key);
		if (faceTex)
		{
			faceTex->SetApplyCenter(false);
		}
	}

	// 객체생성전에 미리 풀을 생성해둔다. (정지 후 재발동 탄은 화면 밖으로 안 나가도 오래 살아있어서 2000개로 상향)
	_bulletPool.Init(3000);
	_enemyPool.Init(1000);
	_itemPool.Init(2000);
	// 9페이즈(과거를 새기는 시계) 프로펠러가 날개 4장 x 2세트 x 32세그먼트 = 256개를 동시에 쓰므로 300으로 상향.
	_laserSegmentPool.Init(300); // 동시에 존재 가능한 칼날 개수 x 칼날당 세그먼트 개수 기준으로 여유있게. 부족해지면 올릴 것

	// Scene에 필요한 객체 생성
	createObjects();

	if (_startInExtra)
	{
		// createObjects()가 배경을 일단 World_BG(스테이지1용)로 깔아놔서, 2초짜리 지연 타이머를 쓰면
		// 그 사이에 스테이지1 배경이 잠깐 보였다 바뀌는 것처럼 보인다. 대기 없이 바로 전환한다.
		_state = GameSceneState::Extra;
		_extraItemsSpawned = false;
		if (_bgLayer1) _bgLayer1->ChangeTexture(L"Stage3BossBG", 0.f, 0.f);
		if (_bgLayer2) _bgLayer2->ChangeTexture(L"Stage3BossBG", 0.f, 0.f);
		AudioManager::GetInstance().PlayBGM(L"ExtraBGM");
	}
	else
	{
		TimeManager::GetInstance().AddTimer([this](){ _state = GameSceneState::Playing; AudioManager::GetInstance().PlayBGM(L"Stage1WaveBGM"); showStageBanner(1); }, 2.0f, false);
	}
	// Grid 미리 생성
	_gridCountX = (int32)_mapSize.x / _gridSize;
	_gridCountY = (int32)_mapSize.y / _gridSize;

	int32 totalGridCount =_gridCountX * _gridCountY;
	_grid.resize(totalGridCount);

	fs::path fontPath = ResourceManager::GetInstance().GetResourcePath() / L"Fonts/Griun_Fromsol-Rg.ttf";
	AddFontResourceExW(fontPath.c_str(), FR_PRIVATE, 0);
	_uiFont = CreateFont(-22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"Griun Fromsol");
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

	if (_uiFont)
	{
		DeleteObject(_uiFont);
		_uiFont = nullptr;
	}
	fs::path fontPath = ResourceManager::GetInstance().GetResourcePath() / L"Fonts/Griun_Fromsol-Rg.ttf";
	RemoveFontResourceExW(fontPath.c_str(), FR_PRIVATE, 0);
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

	// 일시정지 중엔 여기까지 오지 않으므로(위에서 return), 여기서 타이머(보스 발사 타이머 등)를 처리해야
	// 일시정지 중 탄이 계속 나가는 걸 막을 수 있다.
	// Continue(컨티뉴 확인) 중에도 액터 업데이트 루프가 멈춰서 총알이 안 움직이므로, 타이머까지 같이 멈춰야
	// 보스의 연속발사 타이머가 총알을 계속 만들어놓고 방치하는 걸 막을 수 있다.
	if (_state != GameSceneState::Continue)
	{
		TimeManager::GetInstance().UpdateTimers();
	}

	if (_stageBannerTimer > 0.f)
	{
		_stageBannerTimer -= deltaTime;
	}

	if (_bombFaceTimer > 0.f)
	{
		_bombFaceTimer -= deltaTime;
	}

	if (_state != GameSceneState::Continue)
	{
		for (auto actor : _actors)
		{
			actor->Update(deltaTime);
		}
	}

	/* 디버그 스테이지 점프 키(KEY_1~4). 필요하면 다시 주석 풀어서 쓸 것.
	if (InputManager::GetInstance().GetButtonDown(KeyType::KEY_1))
	{
		if (_boss) _boss->Destroy();
		_bossSpawned = false;
		_boss = nullptr;
		clearWaveActors();
		_state = GameSceneState::Playing;
		_stageElapsedTime = 0.f;
		_nextWaveIndex = 0;
		if (_bgLayer1) _bgLayer1->ChangeTexture(L"World_BG");
		if (_bgLayer2) _bgLayer2->ChangeTexture(L"World_BG");
		AudioManager::GetInstance().PlayBGM(L"Stage1WaveBGM");
		showStageBanner(1);
	}
	if (InputManager::GetInstance().GetButtonDown(KeyType::KEY_2))
	{
		if (_boss) _boss->Destroy();
		_bossSpawned = false;
		_boss = nullptr;
		clearWaveActors();
		_state = GameSceneState::Stage2;
		_stageElapsedTime = 0.f;
		_nextStage2WaveIndex = 0;
		if (_bgLayer1) _bgLayer1->ChangeTexture(L"Stage2BG", 15.f, 0.f);
		if (_bgLayer2) _bgLayer2->ChangeTexture(L"Stage2BG", 15.f, 0.f);
		AudioManager::GetInstance().PlayBGM(L"Stage2WaveBGM");
		showStageBanner(2);
	}
	if (InputManager::GetInstance().GetButtonDown(KeyType::KEY_3))
	{
		if (_boss) _boss->Destroy();
		_bossSpawned = false;
		_boss = nullptr;
		clearWaveActors();
		_state = GameSceneState::Stage3;
		_stageElapsedTime = 0.f;
		_nextStage3WaveIndex = 0;
		_stage3Wave2 = false;
		if (_bgLayer1) _bgLayer1->ChangeTexture(L"Stage3BG", 0.f, 20.f);
		if (_bgLayer2) _bgLayer2->ChangeTexture(L"Stage3BG", 0.f, 20.f);
		AudioManager::GetInstance().PlayBGM(L"Stage3WaveBGM");
		showStageBanner(3);
	}
	if (InputManager::GetInstance().GetButtonDown(KeyType::KEY_4))
	{
		if (_boss) _boss->Destroy();
		_bossSpawned = false;
		_boss = nullptr;
		clearWaveActors();
		_state = GameSceneState::Extra;
		_extraItemsSpawned = false;
		// 웨이브/보스 구간 모두 3스테이지 보스방 배경을 그대로 쓴다.
		if (_bgLayer1) _bgLayer1->ChangeTexture(L"Stage3BossBG", 0.f, 0.f);
		if (_bgLayer2) _bgLayer2->ChangeTexture(L"Stage3BossBG", 0.f, 0.f);
		// 웨이브/보스 구간 모두 같은 BGM을 그대로 쓴다.
		AudioManager::GetInstance().PlayBGM(L"ExtraBGM");
	}
	*/


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
			else if (_nextWaveIndex >= (int32)std::size(g_waveTable) && _enemyPool.GetActiveCount() == 0)
			{
				// 모든 웨이브를 소진하고, 남아있던 일반 요정도 전부 죽은 뒤에 보스 스테이지로 전환
				_state = GameSceneState :: Boss;
				AudioManager::GetInstance().PlayBGM(L"Stage1BossBGM");
			}
			break;
		case GameSceneState :: Boss :
			// Boss 상태에 처음 들어온 프레임에만 스폰 (한 번만 생성)
			if (!_bossSpawned)
			{
					Boss* boss = new Boss();
					// 1스테이지 보스만 탄수 2배 / 탄속 절반
					// 모든 탄을 etama3.png y=48의 구슬 스프라이트로 교체, 패턴별로 다른 색(왼쪽에서 2/4/6번째):
					// Fan=빨강, Circle=자홍, Spiral=파랑. 원형 구슬이라 방향 회전은 필요 없음(faceDirection 안 씀).
					boss->Init(Vector(GWinSizeX * 0.5f, -50.f), L"Boss", g_stage1BossPhases, 200, 2.0f, 0.5f,
							   2, 10.f, 0.1f,
							   8, 100.f, true,
							   60.f, 5,
							   12, 250.f, 1.0f, 0.8f,
							   20, 30.f, 200.f, 400.f,
							   12, L"OrbMagenta",
							   L"", -1.f,
							   -1,
							   L"", -1, -1.f,
							   L"", L"OrbBlue", L"OrbRed", L"");
					_reservedAdd.push_back(boss);
					_boss = boss;
					_bossSpawned = true;
					ShowFace(L"Boss1Face");
			}
			else if (_player == nullptr)
			{
				onPlayerDead();
			}
			else if(_boss == nullptr)
			{
				// Stage1의 _stageElapsedTime을 그대로 물려받으면 g_stage2WaveTable의
				// time 값과 어긋나므로 Stage2 시작 시점 기준으로 리셋한다.
				BombClearBullets();
				_stageElapsedTime = 0.f;
				if (_bgLayer1) _bgLayer1->ChangeTexture(L"Stage2BG", 15.f, 0.f);
				if (_bgLayer2) _bgLayer2->ChangeTexture(L"Stage2BG", 15.f, 0.f);
				_stateAfterResult = GameSceneState::Stage2;
				_state = GameSceneState::StageResult;
			}

			break;
		case GameSceneState :: StageResult :
			if (InputManager::GetInstance().GetButtonDown(KeyType::ATTACK))
			{
				_state = _stateAfterResult;
				// StageResult는 Stage2/Stage3 웨이브 시작 전 공통 경유 화면이라, 다음 웨이브에 맞는 BGM을 여기서 튼다.
				if (_stateAfterResult == GameSceneState::Stage2)
				{
					AudioManager::GetInstance().PlayBGM(L"Stage2WaveBGM");
					showStageBanner(2);
				}
				else if (_stateAfterResult == GameSceneState::Stage3)
				{
					AudioManager::GetInstance().PlayBGM(L"Stage3WaveBGM");
					showStageBanner(3);
				}
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
			else if (_nextStage2WaveIndex >= (int32)std::size(g_stage2WaveTable) && _enemyPool.GetActiveCount() == 0)
			{
				// 모든 웨이브를 소진하고, 남아있던 일반 요정도 전부 죽은 뒤에 Stage2 보스로 전환
				_bossSpawned = false;
				_state = GameSceneState :: Stage2Boss;
				AudioManager::GetInstance().PlayBGM(L"Stage2BossBGM");
			}
			break;
		case GameSceneState :: Stage2Boss :
			// Boss 상태와 동일한 패턴: 처음 들어온 프레임에만 스폰, 죽으면 Clear로 전환
			if (!_bossSpawned)
			{
				Boss* boss = new Boss();
				// 2스테이지 보스도 탄수 2배 / 탄속 절반
				// 모든 탄을 etama3.png y=64의 꽃잎 스프라이트로 교체, 패턴별로 다른 색:
				// AimedBurst=흰색, Circle=빨강, Random=보라, CircleDelayedAimed=파랑, CircleDelayedRandom=초록.
				// 꽃잎 텍스처(회전 캔버스 24x24, 실제 그림은 14x16)라 콜라이더는 반지름 7로 맞춤.
				boss->Init(Vector(GWinSizeX * 0.5f, -50.f), L"Boss2", g_stage2BossPhases, 1200, 2.0f, 0.5f,
						   2, 10.f, 0.1f,
						   8, 100.f, true,
						   60.f, 5,
						   12, 250.f, 1.0f, 0.8f,
						   20, 30.f, 200.f, 400.f,
						   12, L"PetalRed",
						   L"PetalWhite", 7.f,
						   -1,
						   L"", -1, -1.f,
						   L"PetalPurple", L"", L"", L"",
						   true, 7.f,
						   L"PetalBlue", L"PetalGreen");
				_reservedAdd.push_back(boss);
				_boss = boss;
				_bossSpawned = true;
				ShowFace(L"Boss2Face");
				// 스크롤 없는 고정 배경으로 교체 (화면 크기에 맞춰 미리 늘려둔 이미지)
				if (_bgLayer1) _bgLayer1->ChangeTexture(L"Stage2BossBG", 0.f, 0.f);
				if (_bgLayer2) _bgLayer2->ChangeTexture(L"Stage2BossBG", 0.f, 0.f);
			}
			else if (_player == nullptr)
			{
				onPlayerDead();
			}
			else if (_boss == nullptr)
			{
				BombClearBullets();

				// Stage3도 Stage1->Stage2 전환과 마찬가지로 경과시간/웨이브 인덱스를 리셋한다.
				_stageElapsedTime = 0.f;
				_nextStage3WaveIndex = 0;
				_stage3Wave2 = false;
				// 웨이브/중간보스 구간은 달 없는 문양 배경만 사용 (본보스 등장 시에만 달 배경으로 교체)
				if (_bgLayer1) _bgLayer1->ChangeTexture(L"Stage3BG", 0.f, 20.f);
				if (_bgLayer2) _bgLayer2->ChangeTexture(L"Stage3BG", 0.f, 20.f);
				_stateAfterResult = GameSceneState::Stage3;
				_state = GameSceneState::StageResult;
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
				else if (_nextStage3WaveIndex >= (int32)std::size(g_stage3Wave1Table) && _enemyPool.GetActiveCount() == 0)
				{
					_bossSpawned = false;
					_state = GameSceneState :: Stage3MidBoss;
					AudioManager::GetInstance().PlayBGM(L"Stage3MidBossBGM");
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
				else if (_nextStage3WaveIndex >= (int32)std::size(g_stage3Wave2Table) && _enemyPool.GetActiveCount() == 0)
				{
					_bossSpawned = false;
					_state = GameSceneState :: Stage3Boss;
					AudioManager::GetInstance().PlayBGM(L"Stage3BossBGM");
				}
			}
			break;
		case GameSceneState :: Stage3MidBoss :
			if (!_bossSpawned)
			{
				Boss* boss = new Boss();
				// Spiral: 팔 1개(한 줄기), 0.0133초 간격으로 촘촘하게(탄수 1.5배), 15도씩 회전
				// Random(1페이즈): 150발, 가감속 번갈아 없이 항상 가속(100.f)
				// Fan(3페이즈): 180도, 10발(2배) / RandomDelayedRandom(3페이즈): 기본값 그대로
				// 모든 탄을 etama3.png y=160의 단검 스프라이트로 교체, 페이즈 등장 순서(AimedBurst -> Random -> Spiral -> Fan -> RandomDelayedRandom)대로
				// 다른 색(흰색/빨강/자홍/파랑/청록)을 준다. 단검 텍스처(32x32)는 콜라이더도 그려지는 칼날 크기(반지름 15)로 맞춤.
				boss->Init(Vector(GWinSizeX * 0.5f, -50.f), L"MidBoss", g_stage3MidBossPhases, 520,
						   1.0f, 1.0f, 1, 15.f, 0.0133f, 120, 100.f, false,
						   180.f, 10,
						   12, 250.f, 1.0f, 0.8f,
						   20, 30.f, 200.f, 400.f,
						   12, L"",
						   L"DaggerWhite", -1.f,
						   -1,
						   L"", -1, -1.f,
						   L"DaggerRed", L"DaggerMagenta", L"DaggerBlue", L"DaggerCyan", true, 15.f);
				_reservedAdd.push_back(boss);
				_boss = boss;
				_bossSpawned = true;
				ShowFace(L"Boss3MidFace");
			}
			else if (_player == nullptr)
			{
				onPlayerDead();
			}
			else if (_boss == nullptr)
			{
				BombClearBullets();

				// 중간보스 격파 -> 후반부 웨이브 재생을 위해 경과시간/인덱스를 리셋한다.
				_stageElapsedTime = 0.f;
				_nextStage3WaveIndex = 0;
				_stage3Wave2 = true;
				_state = GameSceneState :: Stage3;
				AudioManager::GetInstance().PlayBGM(L"Stage3WaveBGM");
			}
			break;
		case GameSceneState :: Stage3Boss :
			if (!_bossSpawned)
			{
				Boss* boss = new Boss();
				// AimedSpread(1페이즈): 30도 범위로 퍼지며 30발, 1/30초 간격(초당 30발, 절반) — 정확히 1초간 발사 후 자연스레 쉼
				// Circle(1페이즈, Fan 대체): 30발, etama3.png 노란 구슬(y=128, 7번째) 텍스처
				// AimedBurst(2페이즈): etama3.png 초록 고리(y=32, 11번째) 텍스처
				// Circle 콜라이더도 텍스처(30x30) 대비 절반(13.5)로 줄임.
				// decorPhaseIndex=2(Telegraph 페이즈, 3번째)에서만 보스 뒤에 마법진 연출
				// Spiral: 팔1/15도/0.0133초 — 중간보스 2페이즈와 동일 스펙(4페이즈의 Spiral+Cross에도 같이 적용됨, 보스 공용 설정이라)
				boss->Init(Vector(GWinSizeX * 0.5f, -50.f), L"Boss3", g_stage3BossPhases, 2000,
						   1.0f, 1.0f, 1, 15.f, 0.0133f, 8, 100.f, true, 60.f, 5,
						   12, 250.f, 1.0f, 0.8f, 30, 30.f, 200.f, 400.f, 30, L"CircleBulletYellow",
						   L"AimedBurstBullet", 13.5f, 2,
						   // 4페이즈(index 3) AimedBurst만 etama4.png sprite0(64x64) 텍스처+콜라이더(그려지는 원 크기인 반지름 31)로 교체.
						   L"AimedBurstBulletBig", 3, 31.f);
				_reservedAdd.push_back(boss);
				_boss = boss;
				_bossSpawned = true;
				ShowFace(L"Boss3Face");
				// 본보스 등장 시에만 화면 꽉 채운 붉은 달 고정 배경으로 교체
				if (_bgLayer1) _bgLayer1->ChangeTexture(L"Stage3BossBG", 0.f, 0.f);
				if (_bgLayer2) _bgLayer2->ChangeTexture(L"Stage3BossBG", 0.f, 0.f);
			}
			else if (_player == nullptr)
			{
				onPlayerDead();
			}
			else if (_boss == nullptr)
			{
				BombClearBullets();

				_stageElapsedTime = 0.f;
				_clearedExtra = false;
				_state = GameSceneState :: Clear;
			}
			break;
		case GameSceneState :: Extra :
			// 요정 웨이브 없이 FullPower 3개만 배치하고, 전부 회수되면 바로 보스로 전환.
			if (!_extraItemsSpawned)
			{
				SpawnItem(Vector(GWinSizeX * 0.25f, 100.f), ItemKind::Power, 128);
				SpawnItem(Vector(GWinSizeX * 0.5f, 100.f), ItemKind::Power, 128);
				SpawnItem(Vector(GWinSizeX * 0.75f, 100.f), ItemKind::Power, 128);
				_extraItemsSpawned = true;
			}
			if (_player == nullptr)
			{
				onPlayerDead();
			}
			else if (_extraItemsSpawned && _itemPool.GetActiveCount() == 0)
			{
				_bossSpawned = false;
				_state = GameSceneState :: ExtraBoss;
			}
			break;
		case GameSceneState :: ExtraBoss :
			// 1페이즈: "이중 나선 우리". Spiral 팔 3개(0.05초 간격, 회전속도 25)를 기본 회피 루트로 삼고,
			// AimedSpread/Telegraph가 리듬을 깬다. fixedPosPhaseIndex=0이라 이 페이즈 동안 보스는 중앙 고정.
			// 2페이즈: ConvergingBurst — 3발 동시발사, 다른 속도로 시작해서 목표속도로 수렴.
			if (!_bossSpawned)
			{
				Boss* boss = new Boss();
				boss->Init(Vector(GWinSizeX * 0.5f, -50.f), L"ExtraBoss", g_extraBossPhases, 5000, 1.0f, 0.5f,	// bulletSpeedMul 0.5: 1페이즈(Spiral/AimedSpread/Telegraph) 속도 절반
						   2, 25.f, 0.1f,
						   8, 100.f, true,
						   60.f, 5,
						   12, 250.f, 1.0f, 0.8f,
						   30, 25.f, 250.f, 450.f,
						   45, L"",	// circleShotCount 45 x 3발 = 135발
						   L"", -1.f,
						   -1,
						   L"", -1, -1.f,
						   L"", L"", L"", L"",
						   false, -1.f,
						   L"", L"",
						   0, 15.f,
						   150.f, 150.f, 1.2f,	// convergingSpeed=150, convergingSpeedSpread=150(그대로), convergingInterval 0.6 -> 1.2(쿨타임 2배)
						   10.f,
						   4);	// illusionPhaseIndex=4: 5페이즈(g_extraBossPhases[4])에서 분신 3체 소환
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
				BombClearBullets();
				_stageElapsedTime = 0.f;
				_clearedExtra = true;
				_state = GameSceneState :: Clear;
			}
			break;
		case GameSceneState :: Clear :
			SaveManager::GetInstance().UnlockExtra();
			SceneManager::GetInstance().ChangeScene(new EndingScene(_score, _clearedExtra));
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
					player->SetPos(Vector(GWinSizeX * 0.5f, 650));
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

// 팝업(일시정지/컨티뉴) 뒤에 깔아서 플레이 화면을 어둡게 눌러주는 반투명 오버레이.
static void DrawDimOverlay(HDC hdc)
{
	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP memBmp = CreateCompatibleBitmap(hdc, 1, 1);
	HBITMAP prevBmp = (HBITMAP)SelectObject(memDC, memBmp);
	SetPixel(memDC, 0, 0, RGB(0, 0, 0));

	BLENDFUNCTION bf{};
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = 150;
	bf.AlphaFormat = 0;
	AlphaBlend(hdc, 0, 0, GWindowSizeX, GWinSizeY, memDC, 0, 0, 1, 1, bf);

	SelectObject(memDC, prevBmp);
	DeleteObject(memBmp);
	DeleteDC(memDC);
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

	if (_bombFaceTimer > 0.f)
	{
		Texture* face = ResourceManager::GetInstance().GetTexture(_faceTextureKey);
		if (face)
		{
			// 마지막 BOMB_FACE_OUTRO초 동안 스케일 업 + 페이드 아웃.
			float elapsed = BOMB_FACE_DURATION - _bombFaceTimer;
			float outroStart = BOMB_FACE_DURATION - BOMB_FACE_OUTRO;
			float outT = std::clamp((elapsed - outroStart) / BOMB_FACE_OUTRO, 0.f, 1.f);

			float scaleMul = BOMB_FACE_SCALE + outT * BOMB_FACE_OUTRO_SCALE;
			BYTE alpha = (BYTE)(255.f * (1.f - outT));

			float baseWidth = (float)face->GetSizeX();
			float baseHeight = (float)face->GetSizeY();
			Vector destSize(baseWidth * scaleMul, baseHeight * scaleMul);
			// 왼쪽 아래 기준, 기존 위치에서 100px 위로. (원본 크기 * 기본 배율 기준으로 고정)
			Vector pos(10.f, (float)GWinSizeY - baseHeight * BOMB_FACE_SCALE - 10.f - 100.f);

			face->RenderScreen(hdc, pos, Vector(0, 0), destSize, alpha);
		}
	}

	if (_stageBannerTimer > 0.f && !_stageBannerText.empty())
	{
		HFONT prevFont = _uiFont ? (HFONT)SelectObject(hdc, _uiFont) : nullptr;
		COLORREF prevColor = SetTextColor(hdc, RGB(255, 255, 255));
		int32 prevBkMode = SetBkMode(hdc, TRANSPARENT);

		SIZE textSize{};
		GetTextExtentPoint32(hdc, _stageBannerText.c_str(), (int32)_stageBannerText.size(), &textSize);
		::TextOut(hdc, (GWinSizeX - textSize.cx) / 2, (GWinSizeY - textSize.cy) / 2,
			_stageBannerText.c_str(), (int32)_stageBannerText.size());

		SetTextColor(hdc, prevColor);
		SetBkMode(hdc, prevBkMode);
		if (prevFont)
		{
			SelectObject(hdc, prevFont);
		}
	}

	if (_isPaused)
{
    DrawDimOverlay(hdc);

    HFONT prevFont = _uiFont ? (HFONT)SelectObject(hdc, _uiFont) : nullptr;
    COLORREF prevColor = SetTextColor(hdc, RGB(255, 255, 255));
    int32 prevBkMode = SetBkMode(hdc, TRANSPARENT);

    if (_pauseState == PauseState::Menu)
    {
        wstring msg = L"일시정지";
        wstring resumeText = std::format(L"{0} 계속", _pauseMenuSelect == 0 ? L">" : L" ");
        wstring quitText = std::format(L"{0} 게임 나가기", _pauseMenuSelect == 1 ? L">" : L" ");

        ::TextOut(hdc, GWinSizeX/2 - 40, GWinSizeY/2 - 40, msg.c_str(), (int32)msg.size());
        ::TextOut(hdc, GWinSizeX/2 - 40, GWinSizeY/2, resumeText.c_str(), (int32)resumeText.size());
        ::TextOut(hdc, GWinSizeX/2 - 40, GWinSizeY/2 + 40, quitText.c_str(), (int32)quitText.size());
    }
    else if (_pauseState == PauseState::QuitConfirm)
    {
        wstring msg = L"정말?";
        wstring yesText = std::format(L"{0} 네", _quitConfirmSelectYes ? L">" : L" ");
        wstring noText = std::format(L"{0} 아니오", !_quitConfirmSelectYes ? L">" : L" ");

        ::TextOut(hdc, GWinSizeX/2 - 30, GWinSizeY/2 - 40, msg.c_str(), (int32)msg.size());
        ::TextOut(hdc, GWinSizeX/2 - 30, GWinSizeY/2, yesText.c_str(), (int32)yesText.size());
        ::TextOut(hdc, GWinSizeX/2 - 30, GWinSizeY/2 + 40, noText.c_str(), (int32)noText.size());
    }

    SetTextColor(hdc, prevColor);
    SetBkMode(hdc, prevBkMode);
    if (prevFont)
    {
        SelectObject(hdc, prevFont);
    }
}


	if (_state == GameSceneState::Continue)
	{
		DrawDimOverlay(hdc);

		HFONT prevFont = _uiFont ? (HFONT)SelectObject(hdc, _uiFont) : nullptr;
		COLORREF prevColor = SetTextColor(hdc, RGB(255, 255, 255));
		int32 prevBkMode = SetBkMode(hdc, TRANSPARENT);

		wstring msg = L"이어서 계속?";
		wstring yesText = std::format(L"{0} 네", _continueSelectYes ? L">" : L" ");
		wstring noText = std::format(L"{0} 아니오", !_continueSelectYes ? L">" : L" ");

		::TextOut(hdc, GWinSizeX / 2 - 30, GWinSizeY / 2 - 30, msg.c_str(), (int32)msg.size());
		::TextOut(hdc, GWinSizeX / 2 - 30, GWinSizeY / 2 + 10, yesText.c_str(), (int32)yesText.size());
		::TextOut(hdc, GWinSizeX / 2 - 30, GWinSizeY / 2 + 50, noText.c_str(), (int32)noText.size());

		SetTextColor(hdc, prevColor);
		SetBkMode(hdc, prevBkMode);
		if (prevFont)
		{
			SelectObject(hdc, prevFont);
		}
	}

	if (_state == GameSceneState::GameOver)
	{
		wstring msg = L"GAME OVER";
		wstring guide = L"Press Z";

		::TextOut(hdc, GWinSizeX / 2 - 40, GWinSizeY / 2 - 20, msg.c_str(), (int32)msg.size());
		::TextOut(hdc, GWinSizeX / 2 - 40, GWinSizeY / 2, guide.c_str(), (int32)guide.size());
	}
}

void GameScene::RenderOverlay(HDC hdc)
{
	// UIManager::Render()보다 나중에(Game::Render()에서) 호출되어, UI 사이드바 위까지 덮는다.
	if (_state == GameSceneState::StageResult)
	{
		Texture* bg = ResourceManager::GetInstance().GetTexture(L"StageResultBG");
		if (bg)
		{
			// UI 사이드바 영역까지 포함해서 창 전체(GWindowSizeX)를 덮도록 그린다.
			bg->RenderScreen(hdc, Vector(GWindowSizeX / 2.0f, GWinSizeY / 2.0f), Vector(0, 0), Vector((float)GWindowSizeX, (float)GWinSizeY));
		}

		HFONT prevFont = _uiFont ? (HFONT)SelectObject(hdc, _uiFont) : nullptr;
		wstring msg = std::format(L"클리어! 점수 : {}", _score);
		::TextOut(hdc, GWindowSizeX / 2 - 60, GWinSizeY / 2 - 20, msg.c_str(), (int32)msg.size());
		if (prevFont)
		{
			SelectObject(hdc, prevFont);
		}

		wstring prompt = L"Press Z to continue";
		::TextOut(hdc, GWindowSizeX / 2 - 60, GWinSizeY / 2 + 10, prompt.c_str(), (int32)prompt.size());
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

void GameScene::SpawnItem(Vector pos, ItemKind kind, int32 powerValue, bool burst, bool autoCollect)
{
	Item* item = _itemPool.Acquire();
	if ( item == nullptr)
		return;

	item->Init(pos, kind, powerValue, burst, autoCollect);
	_reservedAdd.push_back(item);
}

LaserSegment* GameScene::CreateLaserSegment(Vector pos, int32 radius)
{
	LaserSegment* segment = _laserSegmentPool.Acquire();
	if (segment == nullptr)
		return nullptr;

	segment->Init(radius);
	segment->SetPos(pos);

	_reservedAdd.push_back(segment);
	return segment;
}

Laser* GameScene::CreateLaser(Vector pivot, float angleOffset, float length, int32 segmentCount, int32 segmentRadius)
{
	Laser* laser = new Laser();
	laser->Init(pivot, angleOffset, length, segmentCount, segmentRadius);
	_reservedAdd.push_back(laser);

	return laser;
}

BossIllusion* GameScene::CreateBossIllusion(Vector pos, wstring key, int32 hp, float shootOffset, wstring fanTextureKey)
{
	BossIllusion* illusion = new BossIllusion();
	illusion->Init(pos, key, hp, shootOffset, fanTextureKey);
	_reservedAdd.push_back(illusion);

	return illusion;
}


void GameScene::CreateBullet(Vector pos, BulletType type, Vector dir, float speed, bool isHoming, float turnSpeed, float accel,
							  float preStopTime, float launchDelay, BulletRedirectMode redirectMode, wstring customTextureKey,
							  float colliderSizeOverride, bool faceDirection, float targetSpeed, float lifeTime, float fallAccel,
							  bool reflectOffWalls)
{
	// 어떤 경로로 호출되든(타이머 콜백 포함) 일시정지 중에는 새 총알을 만들지 않는다.
	if (_isPaused)
		return;

	Bullet* bullet = _bulletPool.Acquire(); // new Bullet();


	if(bullet == nullptr)
		return;

	bullet->Init(type, dir, speed, isHoming, turnSpeed, accel, preStopTime, launchDelay, redirectMode, customTextureKey, colliderSizeOverride, faceDirection, targetSpeed, lifeTime, fallAccel, reflectOffWalls);
	bullet->SetPos(pos);

	_reservedAdd.push_back(bullet);
}

void GameScene::FireStraight(Vector pos, BulletType type, Vector dir, float speed, wstring customTextureKey, float colliderSizeOverride, bool faceDirection)
{
	CreateBullet(pos, type, dir, speed, false, 180.f, 0.f, 0.f, 0.f, BulletRedirectMode::None, customTextureKey, colliderSizeOverride, faceDirection);
}

void GameScene::FireAimed(Vector pos, BulletType type,Vector targetPos, float speed)
{
	Vector dir = targetPos - pos;
	dir.Normalize();
	CreateBullet(pos, type, dir, speed);
}

void GameScene::FireCircle(Vector pos, BulletType type, int32 count, float speed,
							float preStopTime, float launchDelay, BulletRedirectMode redirectMode, wstring customTextureKey,
							float colliderSizeOverride, bool faceDirection, float startAngle)
{

	for(int32 i = 0; i < count; ++i)
	{
		float radian = DegreeToRadian(startAngle + i*(360.f/count));
		CreateBullet(pos, type, Vector(cosf(radian), sinf(radian)), speed, false, 180.f, 0.f, preStopTime, launchDelay, redirectMode, customTextureKey, colliderSizeOverride, faceDirection);
	}
}

void GameScene::FireSpiral(Vector pos, BulletType type, int32 count, float speed, float& rotationAngle, float rotationSpeed,
							wstring customTextureKey, float colliderSizeOverride, bool faceDirection)
{
	for(int32 i = 0; i < count; ++i)
		{
			float radian = DegreeToRadian(rotationAngle + (i*(360.f/count)));
			CreateBullet(pos, type, Vector(cosf(radian), sinf(radian)), speed, false, 180.f, 0.f, 0.f, 0.f, BulletRedirectMode::None, customTextureKey, colliderSizeOverride, faceDirection);
		}

	rotationAngle += rotationSpeed; // 다음 호출 때 이어질 수 있도록 각도 누적
}

void GameScene::FireFan(Vector pos, BulletType type, Vector dir, float anglespread, int32 count, float speed,
						 wstring customTextureKey, float colliderSizeOverride, bool faceDirection)
{
	float baseAngle = atan2f(dir.y, dir.x); // dir 방향의 각도 (라디안)

	for(int32 i = 0; i < count; ++i)
	{
		float offsetDeg = i * (anglespread / count) - anglespread / 2.f; // 중심 기준 좌우 대칭 오프셋
		float radian = baseAngle + DegreeToRadian(offsetDeg);
		CreateBullet(pos, type, Vector(cosf(radian), sinf(radian)), speed, false, 180.f, 0.f, 0.f, 0.f, BulletRedirectMode::None, customTextureKey, colliderSizeOverride, faceDirection);
	}
}

void GameScene::FireRandom(Vector pos, BulletType type, int32 count, float speed, float accel,
							float preStopTime, float launchDelay, BulletRedirectMode redirectMode,
							wstring customTextureKey, float colliderSizeOverride, bool faceDirection)
{
	uniform_real_distribution<float> randir(0, 360);
	for(int32 i = 0; i < count; ++i)
	{
		float random_dir = randir(gen);
		float radian = DegreeToRadian(random_dir);
		CreateBullet(pos, type, Vector(cosf(radian), sinf(radian)), speed, false, 180.f, accel, preStopTime, launchDelay, redirectMode, customTextureKey, colliderSizeOverride, faceDirection);
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
void GameScene::FireGrid(Vector origin, BulletType type, Vector dir, int32 raws, int32 cols, float spacingX, float spacingY, float speed, wstring customTextureKey, float colliderSizeOverride)
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
			CreateBullet(spawnPos, type, dir, speed, false, 180.f, 0.f, 0.f, 0.f, BulletRedirectMode::None, customTextureKey, colliderSizeOverride);
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

void GameScene::CreateHitEffect(Vector pos)
{
	Effect* effect = new Effect();
	effect->Init(L"HitImpact");	// 단일 프레임 + dur 0.1초라 재생 끝나면(Effect::Update의 IsEnd 체크) 자동 삭제된다.
	effect->SetPos(pos);

	_reservedAdd.push_back(effect);
}

void GameScene::clearWaveActors()
{
	// 일반 적(요정)들을 전부 삭제 예약. 보스는 각 호출부(KEY_1/2/3 핸들러)가 별도로 처리한다.
	const vector<Actor*>& enemies = GetRenderList(RenderLayer::Enemy);
	for (Actor* actor : enemies)
	{
		actor->Destroy();
	}

	ClearEnemyBullets();
}

void GameScene::showStageBanner(int32 stageNum)
{
	_stageBannerText = std::format(L"스테이지 {0}", stageNum);
	_stageBannerTimer = 2.0f;
}

void GameScene::ShowBombFace()
{
	ShowFace(L"BombFace");
}

void GameScene::ShowFace(wstring textureKey)
{
	_faceTextureKey = textureKey;
	_bombFaceTimer = BOMB_FACE_DURATION;
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

void GameScene::ClearBossIllusions()
{
	// 본체(Boss)도 같은 RenderLayer::Boss/ActorType::Boss라서 dynamic_cast로 분신만 골라낸다.
	const vector<Actor*>& bossLayer = GetRenderList(RenderLayer::Boss);
	for (Actor* actor : bossLayer)
	{
		BossIllusion* illusion = dynamic_cast<BossIllusion*>(actor);
		if (illusion != nullptr)
		{
			illusion->Destroy();
		}
	}
}

void GameScene::BombClearBullets()
{
	// vector 스냅샷: SpawnItem이 _reservedAdd에 넣는 동안 _renderList[Bullet]을
	// 직접 순회하고 있으면 안 되므로, 위치만 먼저 복사해둔다.
	vector<Vector> bulletPositions;
	const vector<Actor*>& bullets = GetRenderList(RenderLayer::Bullet);
	for (Actor* actor : bullets)
	{
		if (actor->GetActorType() == ActorType::EnemyBullet)
		{
			bulletPositions.push_back(actor->GetPos());
			actor->Destroy();
		}
	}

	for (const Vector& pos : bulletPositions)
	{
		SpawnItem(pos, ItemKind::Score, 1, false, true);
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
	player->SetPos(Vector(GWinSizeX * 0.5f, 650));
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
		Vector SpawnPos(GWinSizeX * 0.5f, 50.f);
		for(int32 i = 0; i < 6; ++i)
			Game::GetInstance().GetScene()->SpawnItem(SpawnPos, ItemKind::Power, 128, true);
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
		_playerDeathPos = _player->GetPos();
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
	constexpr float arcRadius = 180.0f;
	int32 half = wave.count / 2;

	for(int32 i = 0; i < wave.count; ++i)
	{
		Enemy* enemy = _enemyPool.Acquire();
		if(nullptr == enemy)
		return;

		// 목표 위치를 -60도~+60도 원호 위에 배치해서, 자리 잡았을 때 호 대형이 되게 한다.
		float t = (wave.count > 1) ? (float)i / (wave.count - 1) : 0.5f;
		float angleRad = (-60.0f + 120.0f * t) * (3.14159f / 180.0f);

		// 호의 중심은 wave.pos.x가 아니라 필드 가로 중앙으로 고정한다.
		// wave.pos.x(=50)는 예전 "맨 왼쪽 시작점" 의미로 쓰이던 값이라, 그대로 중심으로 쓰면
		// 대형 절반이 화면 밖(음수 x)으로 나가버린다.
		constexpr float centerX = GWinSizeX / 2.0f;
		Vector target;
		target.x = centerX + sinf(angleRad) * arcRadius;
		target.y = wave.pos.y - (1.0f - cosf(angleRad)) * arcRadius;

		// 왼쪽 자리는 오른쪽 상단에서, 오른쪽 자리는 왼쪽 상단에서 나와 서로 가로지르며 들어온다.
		EntryDirection entryDir = (i < half) ? EntryDirection::Right : EntryDirection::Left;

		enemy->Init(target, wave.enemyKey, entryDir, wave.hpMultiplier);
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