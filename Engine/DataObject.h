#pragma once

class DataObject
{
public:
	// 무조건, fileName을 반환하게 순수가상함수로 만든다.
	virtual wstring GetFileName() = 0;
	virtual void Load(const json& data) = 0;
};

std::string Utf8ToAnsi(const std::string& utf8Str);
std::wstring Utf8ToWide(const std::string& utf8Str);
