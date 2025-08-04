#pragma once
#include "KamataEngine.h"
#include "Transform.h" 
#include "MapChipField.h"

class TitleScene {
private:
	KamataEngine::WorldTransform titleTransform_;
	KamataEngine::WorldTransform playerTransform_;
	KamataEngine::Model* titleModel_ = nullptr;
	KamataEngine::Model* playerModel_ = nullptr;
	// カメラ
	KamataEngine::Camera* camera_ = nullptr;
	// 終了フラグ
	bool finished_ = false;

public:
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	// デスフラグのゲッター
	bool IsFinished() const { return finished_; }

};
