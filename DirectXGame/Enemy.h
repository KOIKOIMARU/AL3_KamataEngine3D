#pragma once
#include "KamataEngine.h"
#include "MapChipField.h"
#include "Transform.h"
#define NOMINMAX
#include <algorithm>

#undef min
#undef max

class MapChipField;
// 敵
class Enemy {
private:
	// ワールド変換データ
	KamataEngine::WorldTransform worldTransform_;
	// モデル
	KamataEngine::Model* model_ = nullptr;
	// カメラ
	KamataEngine::Camera* camera_ = nullptr;
	// 速度
	KamataEngine::Vector3 velocity_ = {};

	// 歩行の速さ
	static inline const float kWalkSpeed = 0.01f; // 歩行速度

	// 最初の角度[度]
	static inline const float kWalkMotionAngleStart = 0.0f; // 歩行モーションの開始角度
	// 最後の角度[度]
	static inline const float kWalkMotionAngleEnd = 45.0f; // 歩行モーションの終了角度
	// アニメーションの周期となる時間[秒]
	static inline const float kWalkMotionTime = 1.0f; // 歩行モーションの周期時間

	// 経過時間
	float walkTimer_ = 0.0f; // 歩行モーションのタイマー



	public:
	// デストラクタ
	~Enemy();

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
};
