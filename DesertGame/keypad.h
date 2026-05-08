#pragma once
#include <gameobject.h>
#include <string>

class Keypad : public GameObject {
public:
	std::string code;
	int length = 4;
	bool used = false;
	std::function<void()> OnSuccess;
};