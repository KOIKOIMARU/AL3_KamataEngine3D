#pragma once
#include "KamataEngine.h"
#include "Transform.h" 
#include "MapChipField.h"
#include "Enemy.h"  
#define NOMINMAX
#include <algorithm>
#include <vector>
#include <array>
#include <cmath>   // std::sin, std::cos, std::lerp


#undef min
#undef max

class MapChipField;
class Enemy;

// 追加：花火タイプ
enum class FireworkType { Rocket, Burst, Pierce };

// 判定
enum class Judge { Perfect, Good, Offbeat };

// 追加：花火弾
struct Firework {
	KamataEngine::Vector3 pos;
	KamataEngine::Vector3 vel;
	float life;   // 秒
	float radius; // 当たり半径
	FireworkType type;
	// ▼ ここから追加
	int damage = 1;     // 与ダメ（Rocket で伸ばす）
	uint8_t tier = 1;   // Offbeat=0, Good=1, Perfect=2（Burst強度）
	int burstCount = 0; // 爆散の子弾数
	float burstSpd = 12.0f;
	float burstLife = 0.45f;
	float burstRad = 0.50f;
};

/// <summary>
/// 自キャラ
/// </summary>
class Player {
public:
	// マップとの当たり判定情報
	struct CollisionMapInfo {
		bool isHitCeiling = false; // 天井衝突フラグ
		bool isHitGround = false;  // 着地フラグ
		bool isHitWall = false;    // 壁接触フラグ
		Vector3 move = {0.0f, 0.0f, 0.0f}; // 実際に移動できた量
	};

	enum Corner {
		kRightBottom, // 右下
		kLeftBottom,  // 左下
		kRightTop,    // 右上
		kLeftTop,     // 左上
		kNumCorner    // 要素数
	};




	// デストラクタ
	~Player();

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="model"></param>
	/// <param name="textureHandle"></param>
	void Initialize(KamataEngine::Model* model,KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	// ワールドトランスフォームへの const 参照を返す
	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }

	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }

	// マップチップのセッター
	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	void MoveInput();

	void CheckCollisionMap(CollisionMapInfo& info); // スライドの「マップ衝突判定」

	void CheckCollisionTop(CollisionMapInfo& info);
	void CheckCollisionBottom(CollisionMapInfo& info);
	void CheckCollisionRight(CollisionMapInfo& info);
	void CheckCollisionLeft(CollisionMapInfo& info);

	Vector3 CornerPosition(const Vector3& center, Corner corner);

	void ApplyCollisionResult(const CollisionMapInfo& info);

	void HandleCeilingCollision(const CollisionMapInfo& info);

	// 接地状態の切り替え処理
	void SwitchLandingState(const CollisionMapInfo& info);

	void HandleWallCollision(const CollisionMapInfo& info);

	Vector3 GetWorldPosition();

	AABB GetAABB(); // AABBを取得する関数

	void OnCollision(const Enemy* enemy);

	// デスフラグゲッター
	bool IsDead() const { return isDead_; }

	float EaseIn(float start, float end, float t);

	float EaseOut(float start, float end, float t);

	  // 花火の読み取り用
	const std::vector<Firework>& GetFireworks() const { return fireworks_; }
	std::vector<Firework>& GetFireworks() { return fireworks_; } // 必要なら

	// 変身形態（今回はRocket固定でOK）
	void SetForm(FireworkType t) { currentForm_ = t; }

	struct RhythmVis {
		float ringR;
		float targetR;
		float phase01;
	};
	RhythmVis GetRhythmVis() const;

	 bool IsAttackHolding() const { return attackHolding_; } // UI用



// 判定イベント
	 struct UIShotEvent {
		 Judge judge;
		 bool hasAnchor = false;      // 角度が有効か
		 float anchorAngleRad = 0.0f; // UI中心基準のラジアン
	 };

	  // UIショットイベントを取り出してクリア
	 std::vector<UIShotEvent> ConsumeUIShotEvents() {
		 auto out = uiShotEvents_;
		 uiShotEvents_.clear();
		 return out;
	 }

	 int GetHP() const { return hp_; }
	 bool IsInvincible() const { return invTimer_ > 0.0f; }

	 void SyncTransformOnly();

	 // Player.h (public:)
	 void ApplyDamage(int dmg = 1, const KamataEngine::Vector3* fromWorldPos = nullptr);

	     // 兵装切替：Eキーで使う
	 void CycleFormNext();

	    struct BurstRequest {
		 KamataEngine::Vector3 pos;
		 int num;
		 float spd;
		 float life;
		 float rad;
	 };

	 // GameScene側から「爆散予定」を積む用（次フレームSpawn）
	 void QueueBurstAt(const KamataEngine::Vector3& p, int num, float spd, float life, float rad) { pendingBursts_.push_back({p, num, spd, life, rad}); }



 private:
	// ワールド変換データ
	KamataEngine::WorldTransform worldTransform_;
	// モデル
	KamataEngine::Model* model_ = nullptr;
	// カメラ
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::Vector3 velocity_ = {};

	// 向き（左右）
	enum class LRDirection {
		kRight,
		kLeft,
	};
	LRDirection lrDirection_ = LRDirection::kRight;

	float turnFirstRotationY_ = 0.0f;                // 旋回開始時の角度
	float turnTimer_ = 0.0f;                         // 旋回時間
	static inline const float kAcceleration = 0.01f; // 移動速度
	static inline const float kAttenuation = 0.1f;   // 減速率
	static inline const float kLimitRunSpeed = 0.1f; // 最大速度
	static inline const float kTimeRun = 0.3f;       // 旋回時間<秒>

	bool onGround_ = true;                                  // 地面にいるかどうか
	static inline const float kGravityAcceleration = 0.02f; // 重力加速度
	static inline const float kLimitFallSpeed = 0.3f;       // 最大落下速度
	static inline const float kJumpAcceleration = 0.4f;     // ジャンプ速度
	static inline const float kAttenuationLanding = 0.5f;   // 着地時の速度減衰率

	static inline const float kAttenuationWall = 0.3f; // 例（減衰30%）

	// マップチップのポインタ
	MapChipField* mapChipField_ = nullptr;

	// キャラクターの当たり判定サイズ
	static inline const float kWidth = 0.99f;
	static inline const float kHeight = 0.99f;

	// デスフラグ
	bool isDead_ = false;


	    // 追加：花火管理
	std::vector<Firework> fireworks_;
	FireworkType currentForm_ = FireworkType::Rocket;

	float fwCooldown_ = 0.0f;                 // 再使用待ち
	static constexpr float kCD_Rocket = 0.6f; // クールタイム
	static constexpr float kFW_Speed = 18.0f; // 秒基準（以前と同じ体感速度）
	static constexpr float kFW_Life = 1.2f;   // 生存時間

	void SpawnRocket_(); // 1種類だけ先に実装

	// QoL: ジャンプ改善
	float coyoteTimer_ = 0.0f;                         // 地面を離れてからの猶予
	float jumpBuffer_ = 0.0f;                          // 先行入力
	static inline const float kCoyoteTime = 0.10f;     // 秒
	static inline const float kJumpBufferTime = 0.10f; // 秒

	// rhythm_ に追記
	struct Rhythm {
		float bpm = 120.0f;
		float secPerBeat = 0.0f;
		float timer = 0.0f;

		// 可視リング設定
		float rStart = 16.0f;
		float rEnd = 120.0f;
		float rTarget = 84.0f;     // 目標半径（UI上の円）
		float tolPerfectPx = 6.0f; // PERFECT ±px
		float tolGoodPx = 14.0f;   // GOOD    ±px

		// ★押した瞬間にゼロから進むローカル時間
		float holdTimer = 0.0f;
		// 何拍後を狙うか（1拍後がデフォ）
		float targetOffsetBeats = 0.85f; // 1拍の85%あたり

		bool pending = false;
		FireworkType pendingType = FireworkType::Rocket;

		// Rhythm の初期値

		float resetSlackSec = 0.08f;     // 80ms くらいの猶予

	} rhythm_;


	// ▼ long-press用
	bool attackHolding_ = false;
	FireworkType holdingType_ = FireworkType::Rocket;
	bool prevPushAttack_ = false; // 直前フレームの押下状態

	void FireByJudge(FireworkType type, Judge j);
	void SpawnRocketRated_(Judge j);


	std::vector<UIShotEvent> uiShotEvents_;
	void EmitUIEvent_(Judge j, bool hasAnchor = false, float angle = 0.0f) { uiShotEvents_.push_back({j, hasAnchor, angle}); }

	    bool IsBlocked_(const KamataEngine::Vector3& p) const;
	bool IsOutOfWorld_(const KamataEngine::Vector3& p, float margin = 1.0f) const;

	   // --- 被弾まわり ---
	int hp_ = 3;
	float invTimer_ = 0.0f;                   // 無敵残り秒
	static constexpr float kIFrameSec = 1.0f; // 無敵時間
	static constexpr float kKnockX = 0.22f;   // ノックバックX
	static constexpr float kKnockY = 0.35f;   // ノックバックY

	static inline const float kMuzzleForward = kWidth * 0.55f;   // 前方オフセット（そのまま）
	static inline const float kMuzzleYOffset = -kHeight * 0.20f; // ★下げる（例: -30%）

	void TakeHit_(const KamataEngine::Vector3& fromPos);

	    // 新しい発射バリエーション
	void SpawnBurstRated_(Judge j);
	void SpawnPierceRated_(Judge j);

	// 爆散の実体生成（小弾をばら撒く）※ループ外で呼ぶ
	void BurstExplode_(const KamataEngine::Vector3& center, int num = 12, float spd = 12.0f, float life = 0.45f, float rad = 0.50f);

	// その場で追加せずループ後に処理するためのキュー
	std::vector<BurstRequest> pendingBursts_;
};