#pragma once

// Actor를 구성하는 기능들
class Component
{
public:
	// 인터페이스 제공, 실제 구현내용은 자식들이 알아서 작성
	virtual void Update(float deltaTime)	 {}
	virtual void Render(HDC hdc, Vector pos) {}

private:

};

