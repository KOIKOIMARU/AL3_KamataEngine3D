#pragma once
#include "KamataEngine.h"
#include "Player.h"
#include <vector>

// ゲームシーン
class GameScene {
	private:
		uint32_t textureHandle_ = 0;
	    std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;

	public:

	// 3Dモデル
	KamataEngine::Model* model_ = nullptr;
	// モデルブロック
	KamataEngine::Model* modelBlock_ = nullptr;
	// カメラ
	KamataEngine::Camera camera_;
	// デバッグカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;
	// デバッグカメラのフラグ 
	bool isDebugCameraActive = false; 
	// 自キャラ
	Player* player_ = nullptr;

	// デストラクタ
	~GameScene();

	// 初期化
	void Initialize();
	// 更新
	void Update();
	// 描画
	void Draw();

};
