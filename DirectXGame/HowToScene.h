#pragma once
#include "KamataEngine.h"

class HowToScene {
public:
	void Initialize();
	void Update();
	void Draw();
	bool IsFinished() const { return finished_; }

private:
	uint32_t tex_ = 0;
	KamataEngine::Sprite* sprite_ = nullptr;
	bool finished_ = false;
};
