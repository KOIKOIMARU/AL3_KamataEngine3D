#pragma once
#include "KamataEngine.h"
#include "Transform.h" 
#include "MapChipField.h"
#define NOMINMAX
#include <algorithm>

#undef min
#undef max

class MapChipField;
class Enemy;

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
		Vector3 move;              // 実際に移動できた量
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
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

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
};