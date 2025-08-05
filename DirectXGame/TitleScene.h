#pragma once
#include "KamataEngine.h"
#include "Transform.h" 
#include "MapChipField.h"
#include "Fade.h"


class TitleScene {
public:
	enum class Phase { 
		kFadeIn, 
		kMain, 
		kFadeOut };

	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	~TitleScene();

	// デスフラグのゲッター
	bool IsFinished() const { return finished_; }

private:
	Phase phase_ = Phase::kFadeIn;

	KamataEngine::WorldTransform titleTransform_;
	KamataEngine::WorldTransform playerTransform_;
	KamataEngine::Model* titleModel_ = nullptr;
	KamataEngine::Model* playerModel_ = nullptr;
	Fade* fade_ = nullptr;
	// カメラ
	KamataEngine::Camera* camera_ = nullptr;
	// 終了フラグ
	bool finished_ = false;

	const float kFadeDuration = 1.0f; // フェード時間（1秒）

};
