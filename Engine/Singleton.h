#pragma once

// 객체를 1개만 만들수있게 제한하는 역할
// 싱글톤을 만들수 있는 방식은 여러개 있다.
template<typename T>
class Singleton
{
public:
	// 공용적으로 접근할수 있는 함수는 만들어줘야한다.
	static T& GetInstance()	// 객체가 필요없어도 호출되는 함수여야 한다.
	{
		static T instance;	// 생성자가 protected
		return instance;
	}

private:
	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;

protected:
	Singleton() = default;	// 생성하는것 자체를 한명이 담당.
	~Singleton() = default;
};