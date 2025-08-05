#pragma once
#include "CameraController.h"
#include "DeathParticles.h"
#include "Enemy.h"
#include "KamataEngine.h"
#include "MapChipField.h"
#include "Player.h"
#include "Skydome.h"
#include "Fade.h"
#include <vector>

// ゲームシーン
class GameScene {
public:
	// ゲームのフェーズ
	enum class Phase {
		kFadeIn,
		kPlay, 
		kDeath ,
		kFadeOut
	};
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

private:
	uint32_t textureHandle_ = 0;
	// ブロック
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;
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
	// フェード
	Fade* fade_ = nullptr;
	const float kFadeDuration = 1.0f; // 秒数（好みで）


	// 現在のフェーズ
	Phase phase_;

	// 終了フラグ
	bool finished_ = false;
};
