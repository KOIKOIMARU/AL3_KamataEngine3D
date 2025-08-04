#pragma once
#include "KamataEngine.h"
#include "Player.h"
#include "Enemy.h"
#include "Skydome.h"
#include "MapChipField.h"
#include <vector>
#include "CameraController.h"
#include "DeathParticles.h"


// ゲームシーン
class GameScene {
	// ゲームのフェーズ
	enum class Phase{
		kPlay,
		kDeath
	};
	// 現在のフェーズ
	Phase phase_;
	private:
		uint32_t textureHandle_ = 0;
	    std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;

	public:

	// 3Dモデル
	KamataEngine::Model* playerModel_ = nullptr;
	KamataEngine::Model* enemyModel_ = nullptr;
	KamataEngine::Model* particleModel_ = nullptr;
	KamataEngine::Model* modelSkydome_ = nullptr;
	// モデルブロック
	KamataEngine::Model* modelBlock_ = nullptr;
	// カメラ
	KamataEngine::Camera camera_;
	// デバッグカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;
	// デバッグカメラのフラグ 
	bool isDebugCameraActive = false; 
	// 自キャラ
	Player* player_;
	// 敵
	std::list<Enemy*> enemies_;
	// パーティクル
	DeathParticles* deathParticles_ = nullptr;
	// 天球
	Skydome skydome_;
	// マップチップフィールド
	MapChipField* mapChipField_;
	// カメラコントローラー
	CameraController* cameraController_ = nullptr;
	
	// 終了フラグ
	bool finished_ = false;

	// デストラクタ
	~GameScene();

	// 初期化
	void Initialize();
	// 更新
	void Update();
	// 描画
	void Draw();

	void GenerateBlocks();

	// 全ての当たり判定を行う
	void CheckAllCollision();

	inline bool IsCollisionAABB(const AABB& a, const AABB& b);

	void UpdatePlayPhase();

	void UpdateDeathPhase();

	// フェーズの切り替え
	void ChangePhase();

	// デスフラグのゲッター
	bool IsFinished() const { return finished_; }
};
