#pragma once
#include "KamataEngine.h"

/// <summary>
/// 天球
/// </summary>
class Skydome {
private:
	// ワールド変換データ
	KamataEngine::WorldTransform worldTransform_;
	// モデル
	KamataEngine::Model* model_ = nullptr;
	// テクスチャハンドル
	uint32_t textureHandle_ = 0u;
	// カメラ
	KamataEngine::Camera* camera_ = nullptr;

public:

	// 初期化
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, uint32_t textureHandle);
	// 更新
	void Update();
	// 描画
	void Draw();


};