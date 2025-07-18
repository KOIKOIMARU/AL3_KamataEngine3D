#pragma once
#include "KamataEngine.h"
#include "Transform.h" 
#include <algorithm>

/// <summary>
/// 自キャラ
/// </summary>
class Player {
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

	float turnFirstRotationY_ = 0.0f;                 // 旋回開始時の角度
	float turnTimer_ = 0.0f;                         // 旋回時間
	static inline const float kAcceleration = 0.01f; // 移動速度
	static inline const float kAttenuation = 0.98f;  // 減速率
	static inline const float kLimitRunSpeed = 0.1f; // 最大速度
	static inline const float kTimeRun = 0.3f;    // 旋回時間<秒>

	bool onGround_ = true; // 地面にいるかどうか
	static inline const float kGravityAcceleration = 0.02f; // 重力加速度
	static inline const float kLimitFallSpeed = 0.3f;        // 最大落下速度
	static inline const float kJumpAcceleration = 0.3f;             // ジャンプ速度

public:

	// デストラクタ
	~Player();

	/// <summary>
	/// 
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
};