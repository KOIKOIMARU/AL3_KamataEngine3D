#pragma once
#include "KamataEngine.h"

class ResultScene {
public:
	enum class Kind { GameOver, Clear };

	explicit ResultScene(Kind kind = Kind::GameOver) : kind_(kind) {}

	void Initialize();
	void Update();
	void Draw();

	bool IsFinished() const { return finished_; }
	bool WantsRetry() const { return retry_; } // Rでリトライ

private:
	Kind kind_ = Kind::GameOver;

	bool finished_ = false;
	bool retry_ = false;

	// 画像スプライト（タイトルと同じ構成）
	KamataEngine::Sprite* spBG_ = nullptr;    // 背景 1280x720
	KamataEngine::Sprite* spLogo_ = nullptr;  // “GAME OVER / GAME CLEAR” ロゴ(正方形)
	KamataEngine::Sprite* spPress_ = nullptr; // “PRESS SPACE”

	// ハンドル（必要なら参照）
	uint32_t texBG_ = 0, texLogo_ = 0, texPress_ = 0;

	// 点滅用タイマ
	float t_ = 0.0f;

};
