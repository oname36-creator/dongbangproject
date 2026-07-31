#pragma once

#include <iostream>
#include <windows.h>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <functional>
#include <algorithm>
#include <windowsx.h>

// C++ 20 std::format
#include <format>
#include <filesystem>
namespace fs = std::filesystem;

#include "Util.h"

// transparent Blt
#pragma comment(lib, "msimg32.lib")

// PlaySound
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// Json
#include <fstream>
#include "Json/nlohmann/json.hpp" // nlohmann/json 헤더 파일
using json = nlohmann::json;


// 정수형을 관리하기 편한 용도로, 별칭을 만들자. 
// -> 언리얼이 이런방식이라 미리 익숙해지자.

using int8 = char;		// 1byte 정수형
using int16 = short;	// 2byte 정수형
using int32 = int;		// 4byte 정수형
using int64 = long long;// 8byte 정수형

using uint8 = unsigned char;		// 1byte 정수형
using uint16 = unsigned short;	// 2byte 정수형
using uint32 = unsigned int;		// 4byte 정수형
using uint64 = unsigned long long;// 8byte 정수형

//int MAX_COUNT;	
//int32 MAX_COUNT;
//int8 MAX_COUNT

using namespace std;


#define SMALL_NUMBER			(1.e-8f)

struct Vector
{
	float x = 0;
	float y = 0;

	Vector() {}
	Vector(float x, float y) : x(x), y(y) {}
	Vector(POINT pt) : x((float)pt.x), y((float)pt.y) {}

	Vector operator+(const Vector& other)
	{
		Vector ret;
		ret.x = x + other.x;
		ret.y = y + other.y;
		return ret;
	}

	Vector operator-(const Vector& other)
	{
		Vector ret;
		ret.x = x - other.x;
		ret.y = y - other.y;
		return ret;
	}

	Vector operator*(float value)
	{
		Vector ret;
		ret.x = x * value;
		ret.y = y * value;
		return ret;
	}
	void operator+=(const Vector& other)
	{
		x += other.x;
		y += other.y;
	}

	void operator-=(const Vector& other)
	{
		x -= other.x;
		y -= other.y;
	}

	void operator*=(float ratio)
	{
		x *= ratio;
		y *= ratio;
	}

	bool operator==(const Vector& other) const
	{
		return (x == other.x && y == other.y);
	}

	bool operator==(Vector& other)
	{
		return (x == other.x && y == other.y);
	}


	// 내적
	// 외적
	// 정규화
	// 크기
	float LengthSquared()
	{
		return x * x + y * y;
	}
	float Length()
	{
		return ::sqrt(LengthSquared());
	}

	void Normalize()
	{
		float length = Length();
		if (length < SMALL_NUMBER)
			return;

		x /= length;
		y /= length;
	}

	// Dot 내적
	// float = cos(A)
	float Dot(Vector other)
	{
		return x * other.x + y * other.y;
	}

	// Cross 외적
	// 결과가 3D 벡터여야하는데, 2D 좌표계라서 Z=0, float 하나를 리턴시킨다. (0,0,x)
	float Cross(Vector other)
	{
		return x * other.y - y * other.x;
	}

	Vector Rotate(float radian)
	{
		float cosA = cosf(radian);
		float sinA = sinf(radian);

		return Vector(x * cosA - y * sinA, x * sinA + y * cosA);
	}
};

// 윈도우 사이즈 변경
constexpr int32 GWinSizeX = 480;
constexpr int32 GWinSizeY = 800;

// Grid의 한칸 셀 index 정보
struct Cell
{
	int32 iX = 0;
	int32 iY = 0;

	// 유틸 함수
	static Cell ConvertToCell(Vector pos, int32 size)
	{
		if (pos.x < 0 || pos.y < 0)
			return Cell{ -1, -1 };

		return Cell{(int32)pos.x / size, (int32)pos.y / size};
	}
};

// 한칸 그리드안에 걸쳐있는 모든 Actor 관리
struct GridInfo
{
	vector<class Actor*> actors;
};

