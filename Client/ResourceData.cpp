#include "pch.h"
#include "ResourceData.h"

void ResourceData::Load(const json& data)
{
	// json 객체에서 원하는 데이터 필드를 읽어서, 
	// 우리만의 struct 에다가 저장
	auto gameSceneData = data["GameScene"];

	// wstring : 2byte (UTF16)
	for (json::iterator it = gameSceneData.begin(); it != gameSceneData.end(); ++it)
	{
		auto key = it.key(); // "BG", "Player.."
		auto value = it.value();	// { 데이터 }

		Item item;
		item.key = Utf8ToWide(key);
		item.fileName = Utf8ToWide(value["fileName"]);

		if (value.contains("transparent"))
		{
			item.transparent = RGB(value["transparent"][0], value["transparent"][1], value["transparent"][2]);
		}

		if (value.contains("countX"))
		{
			item.countX = value["countX"];
		}

		if (value.contains("countY"))
		{
			item.countY = value["countY"];
		}

		if (value.contains("loop"))
		{
			item.loop = value["loop"];
		}

		if (value.contains("dur"))
		{
			item.dur = value["dur"];
		}

		// json 읽은, 하나의 리소스를 item 구조체로 변환후
		// 관리하는 자료구조에 추가
		_gameSceneData.insert(make_pair(item.key, item));
	}
}
