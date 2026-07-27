#include "pch.h"
#include "DataObject.h"

std::string Utf8ToAnsi(const std::string& utf8Str)
{
	// UTF-8 → UTF-16 (wide string)
	int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
	if (wideLen == 0) return "";

	std::wstring wideStr(wideLen, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], wideLen);

	// UTF-16 → ANSI (CP949)
	int ansiLen = WideCharToMultiByte(949, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
	if (ansiLen == 0) return "";

	std::string ansiStr(ansiLen, 0);
	WideCharToMultiByte(949, 0, wideStr.c_str(), -1, &ansiStr[0], ansiLen, NULL, NULL);

	return ansiStr;
}

std::wstring Utf8ToWide(const std::string& utf8Str)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
	if (len == 0) return L"";

	std::wstring wstr(len, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wstr[0], len);
	wstr.pop_back(); // null 제거

	return wstr;
}
