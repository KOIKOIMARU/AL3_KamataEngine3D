#pragma once
#include "KamataEngine.h"

// ゲームシーン
class GameScene {
private:
	uint32_t textureHandle_ = 0;

	uint32_t soundDataHandle_ = 0;

public:
	// デストラクタ
	~GameScene();

	// スプライト
	KamataEngine::Sprite* sprite_ = nullptr;

	// 3Dモデル
	KamataEngine::Model* model_ = nullptr;

	// ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransform_;

	// カメラ
	KamataEngine::Camera camera_;

	// デバッグカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;

	// 初期化
	void Initialize();
	// 更新
	void Update();
	// 描画
	void Draw();

	// ImGuiで値を入力する変数
	float inputFloat3[3] = {0, 0, 0};

};
