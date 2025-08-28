#pragma once
#include "KamataEngine.h"
#include "Transform.h"
#include "CameraController.h"

class TitleScene {
public:
	void Initialize();
	void Update();
	void Draw();

	bool IsFinished() const { return finished_; }

private:
	KamataEngine::Sprite* spBG_ = nullptr;
	KamataEngine::Sprite* spLogo_ = nullptr;
	KamataEngine::Sprite* spPress_ = nullptr;

	uint32_t texBG_ = 0;
	uint32_t texLogo_ = 0;
	uint32_t texPress_ = 0;

	float t_ = 0.0f;
	bool finished_ = false;

// --- BGM ---
	int bgmData_ = 0;      // waveハンドル
	int bgmVoice_ = -1;    // 再生ボイスID
	float bgmVol_ = 0.65f; // 初期音量
	bool bgmFadeOut_ = false;

};
