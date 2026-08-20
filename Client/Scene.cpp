#include "pch.h"
#include "Scene.h"
#include "Player.h"
#include "Enemy.h"
#include "Background.h"
#include "ResourceManager.h"
#include "TimeManager.h"
#include "Bullet.h"
#include "CollisionManager.h"
#include "Effect.h"
#include "DataManager.h"
#include "ResourceData.h"
#include "WorldBG.h"

// 생성자/소멸자를 cpp 작성하면, Scene의 인스턴스화는 cpp에서 일어남.
// ObjectPool<T> (vector<T>) 값 자체를 가지고 있는 풀을 생성하는것도,
// cpp에서 인스턴스화할때 생성됨.
// 이때는 Bullet/Enemy #include 완료 상태
Scene::Scene() 
{
}
Scene::~Scene()
{
}

