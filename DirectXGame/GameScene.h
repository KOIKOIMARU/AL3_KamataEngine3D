#pragma once
#include "CameraController.h"
#include "DeathParticles.h"
#include "Enemy.h"
#include "KamataEngine.h"
#include "MapChipField.h"
#include "Player.h"
#include "ResultScene.h"
#include "Skydome.h"
#include <algorithm>
#include <cmath>
#include <list>
#include <vector>

struct Spark {
	KamataEngine::Vector3 pos;
	KamataEngine::Vector3 vel;
	float life;
};

class GameScene {
public:
	enum class Phase { kFadeIn, kPlay, kDeath, kFadeOut };

	~GameScene();

	void Initialize();
	void Update();
	void Draw();

	void GenerateBlocks();
	void CheckAllCollision();
	inline bool IsCollisionAABB(const AABB& a, const AABB& b);
	void UpdatePlayPhase();
	void UpdateDeathPhase();
	void ChangePhase();

	bool IsFinished() const { return finished_; }

	ResultScene::Kind GetResultKind() const { return resultKind_; }

	// ★ ここを public に
	enum class ExitTo { None, RetryGame, Title, Result };
	ExitTo GetExitTo() const { return exitTo_; }

	// 爆発ヘルパ
	void SpawnExplosion_(const KamataEngine::Vector3& center, int num, float speed, float lifeSec);

	using ExitHook = std::function<void(ExitTo)>;
	void SetOnExitBegin(ExitHook cb) { onExitBegin_ = std::move(cb); }
	void SetOnExitComplete(ExitHook cb) { onExitComplete_ = std::move(cb); }
	void RequestExit(ExitTo to); // フェード開始

private:
	struct Pickup {
		Vector3 pos;
		float r = 0.4f; // 触れ判定半径
		bool taken = false;
	};
	std::vector<Pickup> pickups_;

	uint32_t textureHandle_ = 0;

	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;
	KamataEngine::Model* playerModel_ = nullptr;
	KamataEngine::Model* enemyModel_ = nullptr;
	KamataEngine::Model* particleModel_ = nullptr;
	KamataEngine::Model* modelSkydome_ = nullptr;
	KamataEngine::Model* modelBlock_ = nullptr;

	KamataEngine::Camera camera_;
	KamataEngine::DebugCamera* debugCamera_ = nullptr;
	bool isDebugCameraActive = false;

	Player* player_ = nullptr;
	std::list<Enemy*> enemies_;
	DeathParticles* deathParticles_ = nullptr;
	Skydome skydome_;
	MapChipField* mapChipField_ = nullptr;
	CameraController* cameraController_ = nullptr;

	Sprite* fgOverlay_ = nullptr;

	KamataEngine::WorldTransform wtMid_;
	KamataEngine::Model* modelMid_ = nullptr;
	uint32_t texMid_ = 0;
	float midParallax_ = 0.6f; // カメラ追従の比率（0=背景寄り, 1=完全追従）
	float midMargin_ = 2.0f;   // 画面外余白

	Phase phase_ = Phase::kPlay;
	bool finished_ = false;

	ResultScene::Kind resultKind_ = ResultScene::Kind::GameOver;

	std::vector<Spark> sparks_;
	KamataEngine::WorldTransform wtFirework_;
	KamataEngine::WorldTransform wtSpark_;
	std::vector<WorldTransform> sparkWTs_;
	std::vector<WorldTransform> fireworkWTs_;

	// ★ ポーズ
	bool paused_ = false;
	KamataEngine::Sprite* pauseOverlay_ = nullptr;
	int pauseSelected_ = 0; // 0:再開,1:リトライ,2:タイトル
	static constexpr int kPauseItems = 3;

	// ★ 終了リクエスト
	ExitTo exitTo_ = ExitTo::None;

	// GameScene.h の private に追加
	uint32_t bossGateX_ = UINT32_MAX; // 最初のG列
	bool bossSpawned_ = false;        // Bからボスを出したか
	uint32_t spawnLookAhead_ = 8;     // 先読みスポーン距離(列)

	// ユーティリティ
	void SpawnAheadEnemies_();
	void CheckBossGateAndSpawnBoss_();

	// ★ ここを private に
	void HandlePauseMenuInput();

	// ★ ストック（最大5）
	static constexpr int kMaxStock = 5;
	int fireworkStock_ = 0;

	// ★ UI用スプライト
	KamataEngine::Sprite* stockSlot_[kMaxStock] = {};

	// ★ ヘルパ
	void AddStock_(int n = 1);
	void UseStock_();
	void ActivateSuperFirework_(); // Qで発動（画面内大爆発）
	void DrawStockUI_();           // ストックUI描画

	KamataEngine::WorldTransform wtPickup_; 
	void CleanupDeadEnemies_();

	// --- GameScene メンバに追加 ---
	static constexpr int kRhythmSeg_ = 64;          // 円の分割数（増やすほど滑らか）
	uint32_t whiteTexRhythm_ = 0;                   // 1x1 白テクスチャ
	std::vector<KamataEngine::Sprite*> ringDots_;   // 伸びるリング
	std::vector<KamataEngine::Sprite*> targetDots_; // 目標リング

// 既存 UIRing に追加
	struct UIRing {
		float t = 0.0f, life = 0.5f;
		float r0 = 0.0f, r1 = 120.0f;
		KamataEngine::Vector4 col{1, 1, 1, 1};
		bool shatter = false; // ★寿命で円周が飛び散る
		bool done = false;    // ★一度だけ実行
	};



	std::vector<UIRing> uiRings_;

	struct UISpark {
		KamataEngine::Vector2 pos, vel;
		float t = 0.0f, life = 0.5f;
		float drag = 0.985f, gravity = 0.0f; // HUDなので0でOK
		KamataEngine::Vector4 col{1, 1, 1, 1};
		float size = 3.0f;
	};
	std::vector<UISpark> uiSparks_;


// GameScene.h の private: 末尾あたり
	void InitRhythmUI_();
	void UpdateRhythmUI_(); // ★追加
	void DrawRhythmUI_();
	void SpawnUIBurst_(Judge j); // ★追加

	struct Lantern {
		Vector3 pos;
		float t = 0.0f;    // 経過
		float life = 2.5f; // 寿命(秒) 適宜
	};
	std::vector<Lantern> lanterns_;
	WorldTransform wtLantern_; // 使い回し

	float superCD_ = 0.0f; // スーパーファイアワーク用の簡易CD

	Sprite* fade_ = nullptr;
	float fadeT_ = 1.0f; // 1=真っ黒, 0=透明
	int fadeDir_ = -1;   // -1:明るく(フェードイン), +1:暗く(フェードアウト), 0:停止
	ExitTo pendingExit_ = ExitTo::None;
	ExitHook onExitBegin_;
	ExitHook onExitComplete_;

	Enemy* bossEnemy_ = nullptr; // 生成したボスを保持
	void CullNonBossEnemies_();

	struct EnemyBullet {
		KamataEngine::Vector3 pos;
		KamataEngine::Vector3 vel;
		float r = 0.20f;   // 当たり半径（タイルが1.0なら 0.2 くらい）
		float life = 3.0f; // 最大寿命(秒) 画面外に出た時の保険
	};

	std::vector<EnemyBullet> enemyBullets_;

	// 弾のWT（再利用用）
	KamataEngine::WorldTransform wtEnemyBullet_;

	// 弾関連
	void SpawnEnemyBullet_(const KamataEngine::Vector3& p, const KamataEngine::Vector3& dir, float sp);
	void UpdateEnemyBullets_(float dt);
	void DrawEnemyBullets_();

	float introMarginTimer_ = 0.0f;

	struct DanmakuBullet {
		KamataEngine::Vector3 pos;
		KamataEngine::Vector3 vel; // フレームあたり
		float r;                   // 半径
		float life;                // 秒
		int bounces;               // 残りバウンド回数
	};

	std::vector<DanmakuBullet> danmakuBullets_;
	KamataEngine::WorldTransform wtDanmaku_; // 描画用

	// 画面端ワールドRect（カメラ位置・射影から復元）
	void GetCameraWorldRect_(float z, float& left, float& right, float& bottom, float& top);

	// 必殺：弾幕生成・更新・描画
	void ActivateDanmakuSuper_();
	void SpawnDanmaku_(int num, float spd, int bounces, float life);
	void UpdateDanmakuBullets_(float dt);
	void DrawDanmakuBullets_();


	    // 必殺ゲージ（0.0〜1.0）
	float spGauge_ = 0.0f;

	// ヒット時のゲージ増加量（調整値）
	static constexpr float kSPGainPerDamage_ = 0.04f;  // 1/3にする
	static constexpr float kSPGainDanmakuHit_ = 0.01f; // こちらも弱め


	// UI（バー）
	KamataEngine::Sprite* spGaugeBack_ = nullptr;
	KamataEngine::Sprite* spGaugeFill_ = nullptr;

	// UIヘルパ
	void AddSP_(float v); // ゲージ加算（Clamp）

	// GameScene.h の private に追記
	// --- ライフUI ---
	uint32_t texHeartOn_ = 0;
	uint32_t texHeartOff_ = 0;
	KamataEngine::Sprite* spHeartOn_ = nullptr;
	KamataEngine::Sprite* spHeartOff_ = nullptr;

	// ハート表示設定
	int lifeIconSize_ = 28; // 1個のサイズ(px)
	int lifeIconGap_ = 6;   // 余白
	int lifeMarginL_ = 16;  // 左上マージンX
	int lifeMarginT_ = 14;  // 左上マージンY

	void DrawLifeUI_(); // 宣言

	// GameScene.h
	std::vector<KamataEngine::Sprite*> lifeOn_;
	std::vector<KamataEngine::Sprite*> lifeOff_;
};