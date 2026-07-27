#pragma once

#include "Singleton.h"

enum class DataType
{
	ResourceData,
	ItemData,
	EnemyData,
	LevelData,
};

class DataObject;

class DataManager : public Singleton<DataManager>
{
	// Singleton 객체를 '친구'로 선언해서 private 접근 가능하게 열어준다.
	friend Singleton<DataManager>;

public:
	void Init(fs::path directory);
	void Load();

	template<typename T>
	T* FindData(wstring key)
	{
		auto find = _datas.find(key);
		if (find != _datas.end())
		{
			return dynamic_cast<T*>(find->second);
		}

		return nullptr;
	}

private:
	void loadDataObject(fs::path directory, wstring key, DataObject* obj);

private:
	// 모든 texture 리소스 폴더내에 있을꺼라, 루트 디렉터리 정보를 미리 만들어두자.
	fs::path _resourcePath;

	// key, data
	// "ResourceData"
	map<wstring, class DataObject*>	 _datas;
};

